#ifndef BUNDLE_BUNDLE_H__
#define BUNDLE_BUNDLE_H__

#include <stdint.h>
#include <string>

namespace bundle {

class FileLock;

struct FileHeader {
  enum {
    kMAGIC_CODE = 0xE4E4E4E4,
    kVERSION = 1,
    kFILE_NORMAL = 1,
    kFILE_REDIRECT = 2,
    kFILE_DELETE = 4,
  };

  uint32_t magic_;
  uint32_t version_;
  uint32_t flag_;
  uint32_t length_; // content length
  char url_[256];
  char user_data_[512 - 256 - 4*4];
};

enum {
  kFileHeaderSize = sizeof(FileHeader),
  kUrlSize = 256,
  kUserDataSize = kFileHeaderSize - kUrlSize - 4 * sizeof(uint32_t),

  // default setting value
  kBundleHeaderSize = 1024,
  kBundleCountPerDay = 50 * 400,
  kMaxBundleSize = 2u * 1024 * 1024 * 1024u,  // 2G
};

// URL util
struct Info {
  int id;
  std::string name;

  size_t offset;
  size_t size;
  std::string datetime; // 20120512/1350
  std::string prefix, postfix;
};

typedef bool (*ExtractUrl)(const char *, Info *);

typedef std::string (*BuildUrl)(const Info &);

#if 0
// 1 simple
// path-to-bundle,offset,size.jpg
//
bool ExtractSimple(const char *url, Info *);
std::string BuildSimple(const Info &);

// 2 with date prefix
// date/path-to-bundle,offset,size.jpg
bool ExtractNormal(const char *url, Info *);
std::string BuildNormal(const Info &);
#endif

struct Setting {
  size_t max_bundle_size;
  int bundle_count_per_day;
  int file_count_level_1;
  int file_count_level_2;

  ExtractUrl extract;
  BuildUrl build;
};

void SetSetting(const Setting& setting);

/*
example:

std::string buf;
int ret = bundle::Reader::Read("p/20120424/E6/SE/Zb1/C5zWed.jpg", &buf, "/mnt/mfs");
*/
class Reader {
public:
  // string as buffer, it's not the best, but simple enough
  static int Read(const std::string &url, std::string *buf, const char *storage
    , ExtractUrl extract = 0, std::string *user_data = 0);

  // if OK return 0, else return error
  static int Read(const char *bundle_file, size_t offset, size_t length
    , char *buf, size_t buf_size, size_t *readed
    , const char *storage
    , char *user_data = 0, size_t user_data_size = 0);

private:
  Reader(const Reader&);
  void operator=(const Reader&);
};

/*
  char *file_buffer = "content of file";
  const int length = strlen(file_buffer);

  Writer *writer = Writer::Allocate("p/20120512", ".jpg", length, "/mnt/mfs");

  std::cout << "write success, url:" << writer->EnsureUrl() << std::endl;
  // p/20120512/1_2_3_4.jpg

  size_t written;
  int ret = writer->Write(file_buffer, length, &written);
  
  writer->Release(); // or delete writer
*/

class Writer {
public:
  // lock_path default is storage/.lock
  static Writer* Allocate(const char *prefix, const char *postfix
    , size_t size, const char *storage
    , const char *lock_path = 0, BuildUrl builder = 0);

  int Write(const char *buf, size_t buf_size
    , size_t *written = 0, const char *user_data = 0, size_t user_data_size = 0) const;

  std::string EnsureUrl() const;

  int Write(const std::string &url, const char *buf, size_t buf_size
    , size_t *written, const char *user_data = 0, size_t user_data_size = 0) const;

  void Release();

  const Info & info() const {
    return info_;
  }
  ~Writer() {
    Release();
  }

private:
  // write a file into the bundle
  static int Write(const std::string &bundle_file, size_t offset, const std::string &url
      , const char *buf, size_t buf_size
      , size_t *written
      , const char *user_data = 0, size_t user_data_size = 0);

  // forbidden create outside
  Writer()
      : filelock_(0), builder_(0)
      {}

  int GetNextBundle(const std::string& storage);

  // non-copy
  Writer(const Writer&);
  void operator=(const Writer&);

  std::string filename_;
  FileLock *filelock_;

  BuildUrl builder_;
  Info info_;
};

// TODO: move in .cc
template<typename T>
inline T Align1K(T v) {
  T a = v % 1024;
  return a ? (v + 1024 - a) : v;
}

}
#endif // BUNDLE_BUNDLE_H__
