/* Copyright (c) 2009 maidsafe.net limited
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    * Neither the name of the maidsafe.net limited nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "boost/scoped_ptr.hpp"
#include "maidsafe-dht/common/crypto.h"
#ifdef __MSVC__
#pragma warning(push)
#pragma warning(disable:4100 4127 4189 4244 4505 4512)
#endif
#include "maidsafe-dht/cryptopp/integer.h"
#include "maidsafe-dht/cryptopp/pwdbased.h"
#include "maidsafe-dht/cryptopp/sha.h"
#include "maidsafe-dht/cryptopp/filters.h"
#include "maidsafe-dht/cryptopp/files.h"
#include "maidsafe-dht/cryptopp/gzip.h"
#include "maidsafe-dht/cryptopp/hex.h"
#include "maidsafe-dht/cryptopp/aes.h"
#include "maidsafe-dht/cryptopp/modes.h"
#include "maidsafe-dht/cryptopp/rsa.h"
#include "maidsafe-dht/cryptopp/osrng.h"
#ifdef __MSVC__
#pragma warning(pop)
#endif
#include "maidsafe-dht/common/platform_config.h"
#include "maidsafe-dht/common/utils.h"
#include "maidsafe-dht/common/log.h"

namespace maidsafe {

namespace crypto {

std::string XOR(const std::string &first, const std::string &second) {
  size_t common_size(first.size());
  if ((common_size != second.size()) || (common_size == 0))
    return "";
  std::string result;
  result.reserve(common_size);
  for (size_t i = 0; i < common_size; ++i)
    result.push_back(first.at(i) ^ second.at(i));
  return result;
}

std::string SecurePassword(const std::string &password,
                           const std::string &salt,
                           const boost::uint32_t &pin) {
  if (password.empty() || salt.empty() || pin == 0)
    return "";
  byte purpose = 0;
  boost::uint16_t iter = (pin % 1000) + 1000;
  CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA512> pbkdf;
  CryptoPP::SecByteBlock derived(AES256_KeySize + AES256_IVSize);
  pbkdf.DeriveKey(derived, derived.size(), purpose,
                  reinterpret_cast<const byte*>(password.data()),
                  password.size(), reinterpret_cast<const byte*>(salt.data()),
                  salt.size(), iter);
  std::string derived_password;
  CryptoPP::StringSink string_sink(derived_password);
  string_sink.Put(derived, derived.size());
  return derived_password;
}

template <>
std::string Hash<SHA1>(const std::string &input) {
  std::string result;
  SHA1 hash;
  CryptoPP::StringSource(input, true,
      new CryptoPP::HashFilter(hash, new CryptoPP::StringSink(result)));
  return result;
}

template <>
std::string Hash<SHA256>(const std::string &input) {
  std::string result;
  SHA256 hash;
  CryptoPP::StringSource(input, true,
      new CryptoPP::HashFilter(hash, new CryptoPP::StringSink(result)));
  return result;
}

template <>
std::string Hash<SHA384>(const std::string &input) {
  std::string result;
  SHA384 hash;
  CryptoPP::StringSource(input, true,
      new CryptoPP::HashFilter(hash, new CryptoPP::StringSink(result)));
  return result;
}

template <>
std::string Hash<SHA512>(const std::string &input) {
  std::string result;
  SHA512 hash;
  CryptoPP::StringSource(input, true,
      new CryptoPP::HashFilter(hash, new CryptoPP::StringSink(result)));
  return result;
}

template <>
std::string HashFile<SHA1>(const boost::filesystem::path &file_path) {
  std::string result;
  SHA1 hash;
  try {
    CryptoPP::FileSource(file_path.string().c_str(), true,
        new CryptoPP::HashFilter(hash, new CryptoPP::StringSink(result)));
  }
  catch(const CryptoPP::Exception &e) {
    DLOG(ERROR) << e.what() << std::endl;
    result.clear();
  }
  return result;
}

template <>
std::string HashFile<SHA256>(const boost::filesystem::path &file_path) {
  std::string result;
  SHA256 hash;
  try {
    CryptoPP::FileSource(file_path.string().c_str(), true,
        new CryptoPP::HashFilter(hash, new CryptoPP::StringSink(result)));
  }
  catch(const CryptoPP::Exception &e) {
    DLOG(ERROR) << e.what() << std::endl;
    result.clear();
  }
  return result;
}

template <>
std::string HashFile<SHA384>(const boost::filesystem::path &file_path) {
  std::string result;
  SHA384 hash;
  try {
    CryptoPP::FileSource(file_path.string().c_str(), true,
        new CryptoPP::HashFilter(hash, new CryptoPP::StringSink(result)));
  }
  catch(const CryptoPP::Exception &e) {
    DLOG(ERROR) << e.what() << std::endl;
    result.clear();
  }
  return result;
}

template <>
std::string HashFile<SHA512>(const boost::filesystem::path &file_path) {
  std::string result;
  SHA512 hash;
  try {
    CryptoPP::FileSource(file_path.string().c_str(), true,
        new CryptoPP::HashFilter(hash, new CryptoPP::StringSink(result)));
  }
  catch(const CryptoPP::Exception &e) {
    DLOG(ERROR) << e.what() << std::endl;
    result.clear();
  }
  return result;
}

std::string SymmEncrypt(const std::string &input,
                        const std::string &key,
                        const std::string &initialisation_vector) {
  if (key.size() < AES256_KeySize ||
      initialisation_vector.size() < AES256_IVSize)
    return "";

  try {
    byte byte_key[AES256_KeySize], byte_iv[AES256_IVSize];

    CryptoPP::StringSource(key.substr(0, AES256_KeySize), true,
        new CryptoPP::ArraySink(byte_key, sizeof(byte_key)));

    CryptoPP::StringSource(initialisation_vector.substr(0, AES256_IVSize), true,
        new CryptoPP::ArraySink(byte_iv, sizeof(byte_iv)));

    std::string result;
    CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption encryptor(byte_key,
        sizeof(byte_key), byte_iv);
    CryptoPP::StringSource(input, true,
        new CryptoPP::StreamTransformationFilter(encryptor,
            new CryptoPP::StringSink(result)));
    return result;
  }
  catch(const CryptoPP::Exception &e) {
    DLOG(ERROR) << e.what() << std::endl;
    return "";
  }
}

std::string SymmDecrypt(const std::string &input,
                        const std::string &key,
                        const std::string &initialisation_vector) {
  if (key.size() < AES256_KeySize ||
      initialisation_vector.size() < AES256_IVSize)
    return "";

  try {
    byte byte_key[AES256_KeySize], byte_iv[AES256_IVSize];

    CryptoPP::StringSource(key.substr(0, AES256_KeySize), true,
        new CryptoPP::ArraySink(byte_key, sizeof(byte_key)));

    CryptoPP::StringSource(initialisation_vector.substr(0, AES256_IVSize), true,
        new CryptoPP::ArraySink(byte_iv, sizeof(byte_iv)));

    std::string result;
    CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption decryptor(byte_key,
        sizeof(byte_key), byte_iv);
    CryptoPP::StringSource(input, true,
        new CryptoPP::StreamTransformationFilter(decryptor,
            new CryptoPP::StringSink(result)));
    return result;
  }
  catch(const CryptoPP::Exception &e) {
    DLOG(ERROR) << e.what() << std::endl;
    return "";
  }
}

CryptoPP::RandomNumberGenerator &GlobalRNG() {
  static CryptoPP::AutoSeededX917RNG<CryptoPP::AES> rand_pool;
  return rand_pool;
}

std::string AsymEncrypt(const std::string &input,
                        const std::string &public_key) {
  try {
    CryptoPP::StringSource key(public_key, true);
    CryptoPP::RSAES_OAEP_SHA_Encryptor encryptor(key);
    std::string result;
    CryptoPP::StringSource(input, true, new CryptoPP::PK_EncryptorFilter(
        GlobalRNG(), encryptor, new CryptoPP::StringSink(result)));
    return result;
  }
  catch(const CryptoPP::Exception &e) {
    DLOG(ERROR) << e.what() << std::endl;
    return "";
  }
}

std::string AsymDecrypt(const std::string &input,
                        const std::string &private_key) {
  if (input.empty())
    return "";
  try {
    CryptoPP::StringSource key(private_key, true);
    CryptoPP::RSAES_OAEP_SHA_Decryptor decryptor(key);
    std::string result;
    CryptoPP::StringSource(input, true, new CryptoPP::PK_DecryptorFilter(
        GlobalRNG(), decryptor, new CryptoPP::StringSink(result)));
    return result;
  }
  catch(const CryptoPP::Exception &e) {
    DLOG(ERROR) << e.what() << std::endl;
    return "";
  }
}

std::string AsymSign(const std::string &input, const std::string &private_key) {
  try {
    CryptoPP::StringSource key(private_key, true);
    CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA512>::Signer signer(key);
    std::string result;
    CryptoPP::StringSource(input, true, new CryptoPP::SignerFilter(GlobalRNG(),
        signer, new CryptoPP::StringSink(result)));
    return result;
  }
  catch(const CryptoPP::Exception &e) {
    DLOG(ERROR) << e.what() << std::endl;
    return "";
  }
}

bool AsymCheckSig(const std::string &input_data,
                  const std::string &input_signature,
                  const std::string &public_key) {
  try {
    CryptoPP::StringSource key(public_key, true);
    CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA512>::Verifier
        verifier(key);
    bool result = false;
    CryptoPP::StringSource signature_string(input_signature, true);
    if (signature_string.MaxRetrievable() != verifier.SignatureLength())
      return result;
    boost::scoped_ptr<CryptoPP::SecByteBlock> signature(
        new CryptoPP::SecByteBlock(verifier.SignatureLength()));
    signature_string.Get(*signature, signature->size());
    CryptoPP::SignatureVerificationFilter *verifier_filter(
        new CryptoPP::SignatureVerificationFilter(verifier));
    verifier_filter->Put(*signature, verifier.SignatureLength());
    CryptoPP::StringSource ssource(input_data, true, verifier_filter);
    result = verifier_filter->GetLastResult();
    return result;
  }
  catch(const CryptoPP::Exception &e) {
    DLOG(ERROR) << "Crypto::AsymCheckSig - " << e.what() << std::endl;
    return false;
  }
}

std::string Compress(const std::string &input,
                     const boost::uint16_t &compression_level) {
  if (compression_level > kMaxCompressionLevel)
    return "";
  try {
    std::string result;
    CryptoPP::StringSource(input, true, new CryptoPP::Gzip(
        new CryptoPP::StringSink(result), compression_level));
    return result;
  }
  catch(const CryptoPP::Exception &e) {
    DLOG(ERROR) << e.what() << std::endl;
    return "";
  }
}

std::string Uncompress(const std::string &input) {
  try {
    std::string result;
    CryptoPP::StringSource(input, true, new CryptoPP::Gunzip(
        new CryptoPP::StringSink(result)));
    return result;
  }
  catch(const CryptoPP::Exception &e) {
    DLOG(ERROR) << e.what() << std::endl;
    return "";
  }
}

void RsaKeyPair::GenerateKeys(const boost::uint16_t &key_size) {
  private_key_.clear();
  public_key_.clear();
  CryptoPP::RandomPool rand_pool;
  std::string seed = SRandomString(key_size);
  rand_pool.IncorporateEntropy(reinterpret_cast<const byte*>(seed.c_str()),
                                                             seed.size());

  CryptoPP::RSAES_OAEP_SHA_Decryptor decryptor(rand_pool, key_size);
  CryptoPP::StringSink private_key(private_key_);
  decryptor.DEREncode(private_key);
  private_key.MessageEnd();

  CryptoPP::RSAES_OAEP_SHA_Encryptor encryptor(decryptor);
  CryptoPP::StringSink public_key(public_key_);
  encryptor.DEREncode(public_key);
  public_key.MessageEnd();
}

}  // namespace crypto

}  // namespace maidsafe
