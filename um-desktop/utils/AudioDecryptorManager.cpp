#include "AudioDecryptorManager.h"
#include "audio_type_sniff.h"

#include "audio_decryptor/KugouMusicDecryptor.h"
#include "audio_decryptor/QQMusicV2Decryptor.h"

#include <boost/filesystem.hpp>

#include <algorithm>
#include <fstream>
#include <vector>

using namespace umc;
using namespace umd;
using namespace umd::utils::audio_decryptor;
namespace nowide = boost::nowide;
namespace fs = boost::filesystem;

namespace umd::utils {

const usize kDecryptBufferSize = 4 * 1024 * 1024;

AudioDecryptorManager::AudioDecryptorManager() {
  Add(std::make_shared<QQMusicV2Decryptor>());
  Add(std::make_shared<KugouMusicDecryptor>());
}

void AudioDecryptorManager::Open(const std::string& file_path) {
  in_file_path_ = file_path;
  Close();
}

void AudioDecryptorManager::Close() {
  if (active_decryptor_) {
    active_decryptor_->Close();
    active_decryptor_ = nullptr;
  }
}

EncryptionType AudioDecryptorManager::SniffEncryption() {
  Close();

  // Check each decryptor we have registered.
  for (auto& decryptor : decryptors_) {
    decryptor->Open(in_file_path_);
    if (decryptor->SetupDecryptor()) {
      active_decryptor_ = decryptor;
      return decryptor->GetEncryptionType();
    }
    decryptor->Close();
  }

  return EncryptionType::kUnsupported;
}

bool AudioDecryptorManager::DecryptAudioFile() {
  if (!active_decryptor_) {
    return false;
  }

  Vec<u8> buf(kAudioTypeSniffBufferSize);
  if (!active_decryptor_->DecryptFirstBlock(buf.data(), buf.size())) {
    return false;
  }

  std::string extension = SniffAudioType(buf.data(), buf.size());
  fs::path in_file_path(in_file_path_);
  in_file_path.replace_extension(extension);
  return active_decryptor_->DecryptEntireFile(in_file_path.string());
}

}  // namespace umd::utils
