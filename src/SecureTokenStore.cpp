#include "SecureTokenStore.h"

#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include <windows.h>
#include <wincrypt.h>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Crypt32.lib")

namespace {

constexpr DWORD kRsaKeyBits = 4096;
const wchar_t *kKeyContainerName = L"RGMT_TokenVault_RSA4096";

QString appDataDir()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir;
}

// Acquires (optionally creating) the persistent CSP key container that
// holds our RSA-4096 AT_KEYEXCHANGE key pair.
bool acquireContext(HCRYPTPROV *hProv, bool allowCreate, QString *error)
{
    if (CryptAcquireContextW(hProv, kKeyContainerName, MS_ENH_RSA_AES_PROV_W, PROV_RSA_AES, 0))
        return true;

    if (!allowCreate) {
        if (error) *error = QStringLiteral("No token vault key found yet.");
        return false;
    }

    if (CryptAcquireContextW(hProv, kKeyContainerName, MS_ENH_RSA_AES_PROV_W, PROV_RSA_AES,
                              CRYPT_NEWKEYSET)) {
        return true;
    }

    if (error)
        *error = QStringLiteral("CryptAcquireContextW(CRYPT_NEWKEYSET) failed: 0x%1")
                     .arg(GetLastError(), 0, 16);
    return false;
}

// Gets the RSA exchange key pair from the container, generating a fresh
// 4096-bit pair on first use.
bool ensureRsaKeyPair(HCRYPTPROV hProv, HCRYPTKEY *hRsaKey, QString *error)
{
    if (CryptGetUserKey(hProv, AT_KEYEXCHANGE, hRsaKey))
        return true;

    // High word of the flags parameter is the key length in bits, per
    // CryptGenKey's documented convention for RSA key generation.
    if (CryptGenKey(hProv, AT_KEYEXCHANGE, (kRsaKeyBits << 16) | CRYPT_EXPORTABLE, hRsaKey))
        return true;

    if (error)
        *error = QStringLiteral("CryptGenKey(RSA-4096) failed: 0x%1").arg(GetLastError(), 0, 16);
    return false;
}

QByteArray dpapiProtect(const QByteArray &plain, QString *error)
{
    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(plain.constData()));
    in.cbData = static_cast<DWORD>(plain.size());
    DATA_BLOB out{};

    if (!CryptProtectData(&in, L"RGMT GitHub token vault", nullptr, nullptr, nullptr,
                           CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        if (error) *error = QStringLiteral("CryptProtectData failed: 0x%1").arg(GetLastError(), 0, 16);
        return {};
    }

    const QByteArray result(reinterpret_cast<const char *>(out.pbData), int(out.cbData));
    LocalFree(out.pbData);
    return result;
}

QByteArray dpapiUnprotect(const QByteArray &sealed, QString *error)
{
    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(sealed.constData()));
    in.cbData = static_cast<DWORD>(sealed.size());
    DATA_BLOB out{};

    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr,
                             CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        if (error) *error = QStringLiteral("CryptUnprotectData failed: 0x%1").arg(GetLastError(), 0, 16);
        return {};
    }

    const QByteArray result(reinterpret_cast<const char *>(out.pbData), int(out.cbData));
    LocalFree(out.pbData);
    return result;
}

} // namespace

QString SecureTokenStore::vaultFilePath()
{
    return QDir(appDataDir()).filePath("token.vault");
}

bool SecureTokenStore::hasToken()
{
    return QFile::exists(vaultFilePath());
}

bool SecureTokenStore::saveToken(const QString &token, QString *error)
{
    HCRYPTPROV hProv = 0;
    if (!acquireContext(&hProv, /*allowCreate=*/true, error))
        return false;

    HCRYPTKEY hRsaKey = 0;
    if (!ensureRsaKeyPair(hProv, &hRsaKey, error)) {
        CryptReleaseContext(hProv, 0);
        return false;
    }

    BYTE iv[16];
    if (!CryptGenRandom(hProv, sizeof(iv), iv)) {
        if (error) *error = QStringLiteral("CryptGenRandom failed: 0x%1").arg(GetLastError(), 0, 16);
        CryptDestroyKey(hRsaKey);
        CryptReleaseContext(hProv, 0);
        return false;
    }

    HCRYPTKEY hAesKey = 0;
    if (!CryptGenKey(hProv, CALG_AES_256, CRYPT_EXPORTABLE, &hAesKey)) {
        if (error) *error = QStringLiteral("CryptGenKey(AES-256) failed: 0x%1").arg(GetLastError(), 0, 16);
        CryptDestroyKey(hRsaKey);
        CryptReleaseContext(hProv, 0);
        return false;
    }

    DWORD mode = CRYPT_MODE_CBC;
    CryptSetKeyParam(hAesKey, KP_MODE, reinterpret_cast<BYTE *>(&mode), 0);
    CryptSetKeyParam(hAesKey, KP_IV, iv, 0);

    // Encrypt the token bytes with the AES-256 session key (PKCS#7 padded).
    const QByteArray tokenBytes = token.toUtf8();
    DWORD cipherLen = static_cast<DWORD>(tokenBytes.size());
    const DWORD bufLen = cipherLen + 32; // headroom for padding
    QByteArray cipherBuf(int(bufLen), 0);
    memcpy(cipherBuf.data(), tokenBytes.constData(), size_t(tokenBytes.size()));

    if (!CryptEncrypt(hAesKey, 0, TRUE, 0, reinterpret_cast<BYTE *>(cipherBuf.data()),
                       &cipherLen, bufLen)) {
        if (error) *error = QStringLiteral("CryptEncrypt failed: 0x%1").arg(GetLastError(), 0, 16);
        CryptDestroyKey(hAesKey);
        CryptDestroyKey(hRsaKey);
        CryptReleaseContext(hProv, 0);
        return false;
    }
    cipherBuf.resize(int(cipherLen));

    // Wrap (RSA-4096 encrypt) the AES-256 key with our public exchange key.
    DWORD wrappedLen = 0;
    CryptExportKey(hAesKey, hRsaKey, SIMPLEBLOB, 0, nullptr, &wrappedLen);
    QByteArray wrappedKey(int(wrappedLen), 0);
    if (!CryptExportKey(hAesKey, hRsaKey, SIMPLEBLOB, 0,
                         reinterpret_cast<BYTE *>(wrappedKey.data()), &wrappedLen)) {
        if (error)
            *error = QStringLiteral("CryptExportKey(SIMPLEBLOB) failed: 0x%1").arg(GetLastError(), 0, 16);
        CryptDestroyKey(hAesKey);
        CryptDestroyKey(hRsaKey);
        CryptReleaseContext(hProv, 0);
        return false;
    }

    CryptDestroyKey(hAesKey);
    CryptDestroyKey(hRsaKey);
    CryptReleaseContext(hProv, 0);

    // Package layout: [ivLen u32][iv][wrappedLen u32][wrappedKey][cipherLen u32][cipher]
    QByteArray package;
    QDataStream out(&package, QIODevice::WriteOnly);
    out << quint32(sizeof(iv));
    out.writeRawData(reinterpret_cast<const char *>(iv), sizeof(iv));
    out << quint32(wrappedKey.size());
    out.writeRawData(wrappedKey.constData(), wrappedKey.size());
    out << quint32(cipherBuf.size());
    out.writeRawData(cipherBuf.constData(), cipherBuf.size());

    const QByteArray sealed = dpapiProtect(package, error);
    if (sealed.isEmpty())
        return false;

    QFile file(vaultFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = QStringLiteral("Could not write %1").arg(vaultFilePath());
        return false;
    }
    file.write(sealed);
    file.close();
    return true;
}

bool SecureTokenStore::loadToken(QString *tokenOut, QString *error)
{
    QFile file(vaultFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("No saved token.");
        return false;
    }
    const QByteArray sealed = file.readAll();
    file.close();

    const QByteArray package = dpapiUnprotect(sealed, error);
    if (package.isEmpty())
        return false;

    QDataStream in(package);
    quint32 ivLen = 0, wrappedLen = 0, cipherLen = 0;
    QByteArray iv, wrappedKey, cipher;

    in >> ivLen;
    iv.resize(int(ivLen));
    in.readRawData(iv.data(), int(ivLen));
    in >> wrappedLen;
    wrappedKey.resize(int(wrappedLen));
    in.readRawData(wrappedKey.data(), int(wrappedLen));
    in >> cipherLen;
    cipher.resize(int(cipherLen));
    in.readRawData(cipher.data(), int(cipherLen));

    HCRYPTPROV hProv = 0;
    if (!acquireContext(&hProv, /*allowCreate=*/false, error))
        return false;

    HCRYPTKEY hRsaKey = 0;
    if (!CryptGetUserKey(hProv, AT_KEYEXCHANGE, &hRsaKey)) {
        if (error)
            *error = QStringLiteral("No RSA key found in vault container: 0x%1").arg(GetLastError(), 0, 16);
        CryptReleaseContext(hProv, 0);
        return false;
    }

    HCRYPTKEY hAesKey = 0;
    if (!CryptImportKey(hProv, reinterpret_cast<const BYTE *>(wrappedKey.constData()),
                         DWORD(wrappedKey.size()), hRsaKey, 0, &hAesKey)) {
        if (error)
            *error = QStringLiteral("CryptImportKey(SIMPLEBLOB) failed: 0x%1").arg(GetLastError(), 0, 16);
        CryptDestroyKey(hRsaKey);
        CryptReleaseContext(hProv, 0);
        return false;
    }

    DWORD mode = CRYPT_MODE_CBC;
    CryptSetKeyParam(hAesKey, KP_MODE, reinterpret_cast<BYTE *>(&mode), 0);
    CryptSetKeyParam(hAesKey, KP_IV, reinterpret_cast<BYTE *>(iv.data()), 0);

    DWORD plainLen = DWORD(cipher.size());
    if (!CryptDecrypt(hAesKey, 0, TRUE, 0, reinterpret_cast<BYTE *>(cipher.data()), &plainLen)) {
        if (error) *error = QStringLiteral("CryptDecrypt failed: 0x%1").arg(GetLastError(), 0, 16);
        CryptDestroyKey(hAesKey);
        CryptDestroyKey(hRsaKey);
        CryptReleaseContext(hProv, 0);
        return false;
    }

    if (tokenOut)
        *tokenOut = QString::fromUtf8(cipher.constData(), int(plainLen));

    CryptDestroyKey(hAesKey);
    CryptDestroyKey(hRsaKey);
    CryptReleaseContext(hProv, 0);
    return true;
}

bool SecureTokenStore::clearToken()
{
    return QFile::remove(vaultFilePath());
}
