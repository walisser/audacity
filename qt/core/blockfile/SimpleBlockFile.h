/**********************************************************************

  Audacity: A Digital Audio Editor

  SimpleBlockFile.h

  Joshua Haberman

**********************************************************************/

#ifndef __AUDACITY_SIMPLE_BLOCKFILE__
#define __AUDACITY_SIMPLE_BLOCKFILE__

#include "../BlockFile.h"

class DirManager;

struct SimpleBlockFileCache {
   bool active;
   bool needWrite;
   sampleFormat format;
   ArrayOf<char> sampleData, summaryData;

   SimpleBlockFileCache() {}
};

// The AU formats we care about
enum {
   AU_SAMPLE_FORMAT_16 = 3,
   AU_SAMPLE_FORMAT_24 = 4,
   AU_SAMPLE_FORMAT_FLOAT = 6,
};

typedef struct {
   uint32_t magic;      // magic number
   uint32_t dataOffset; // byte offset to start of audio data
   uint32_t dataSize;   // data length, in bytes (optional)
   uint32_t encoding;   // data encoding enumeration
   uint32_t sampleRate; // samples per second
   uint32_t channels;   // number of interleaved channels
} auHeader;

class PROFILE_DLL_API SimpleBlockFile /* not final */ : public BlockFile {
 public:

   // Constructor / Destructor

   /// Create a disk file and write summary and sample data to it
   SimpleBlockFile(const QString &baseFileName,
                   samplePtr sampleData, size_t sampleLen,
                   sampleFormat format,
                   bool allowDeferredWrite = false,
                   bool bypassCache = false );
   /// Create the memory structure to refer to the given block file
   SimpleBlockFile(const QString &existingFile, size_t len,
                   float min, float max, float rms);

   virtual ~SimpleBlockFile();

   // Reading

   /// Read the summary section of the disk file
   bool ReadSummary(ArrayOf<char> &data) override;
   /// Read the data section of the disk file
   size_t ReadData(samplePtr data, sampleFormat format,
                        size_t start, size_t len, bool mayThrow) const override;

   /// Create a NEW block file identical to this one
   BlockFilePtr Copy(const QString &newFileName) override;
   /// Write an XML representation of this file
   void SaveXML(XMLWriter &xmlFile) override;

   DiskByteCount GetSpaceUsage() const override;
   void Recover() override;

   static BlockFilePtr BuildFromXML(DirManager &dm, const QStringMap &attrs);

#if DEPRECATED_AUDIO_CACHE
   bool GetNeedWriteCacheToDisk() override;
   void WriteCacheToDisk() override;

   bool GetNeedFillCache() override { return !mCache.active; }

   void FillCache() /* noexcept */ override;
#endif
   
 protected:

   bool WriteSimpleBlockFile(samplePtr sampleData, size_t sampleLen,
                             sampleFormat format, void* summaryData);
   static bool GetCache();
   void ReadIntoCache();

   SimpleBlockFileCache mCache;

 private:
   mutable sampleFormat mFormat; // may be found lazily
};

#endif
