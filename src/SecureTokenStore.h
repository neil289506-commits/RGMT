#pragma once

#include <QString>

// Stores the user's GitHub Personal Access Token on disk with layered
// encryption:
//   1. AES-256 (CBC) encrypts the token bytes themselves.
//   2. The AES-256 session key is wrapped (RSA-encrypted) with an
//      RSA-4096 key pair kept in a private, per-user CryptoAPI key
//      container (never exported to disk in the clear).
//   3. The whole package (IV + RSA-wrapped AES key + AES ciphertext) is
//      additionally sealed with DPAPI (CryptProtectData), so the vault
//      file is only usable by this Windows account on this machine.
//
// This mirrors Microsoft's documented CryptoAPI "envelope encryption"
// pattern (CryptGenKey + CryptExportKey/CryptImportKey with SIMPLEBLOB),
// with DPAPI added as an extra at-rest sealing layer over the package.
//
// NOTE: written against the documented CryptoAPI call sequence but not
// compiled/run on real Windows in this environment — see README
// "Things I could not verify" before relying on it in production.
class SecureTokenStore
{
public:
    static bool hasToken();
    static bool saveToken(const QString &token, QString *error = nullptr);
    static bool loadToken(QString *tokenOut, QString *error = nullptr);
    static bool clearToken();

private:
    static QString vaultFilePath();
};
