/**********************************************************************

  Audacity: A Digital Audio Editor

  ODDecodeBlockFile.h

  Created by Michael Chinen (mchinen)
  Audacity(R) is copyright (c) 1999-2008 Audacity Team.
  License: GPL v2.  See License.txt.

******************************************************************//**

\class ODDecodeBlockFile
\brief ODDecodeBlockFile is a special type of SimpleBlockFile that does not necessarily have summary OR audio data available
The summary and audio is eventually computed and written to a file in a background thread.

Load On-Demand implementation of the SimpleBlockFile for audio files that need to be decoded (mp3,flac,etc..).

Also, see ODPCMAliasBlockFile for a similar file.
*//*******************************************************************/






#ifndef __AUDACITY_ODDecodeBlockFile__
#define __AUDACITY_ODDecodeBlockFile__

#include "SimpleBlockFile.h"
//#include "../BlockFile.h"
//#include "../ondemand/ODTaskThread.h"
//#include "../DirManager.h"
//#include "../ondemand/ODDecodeTask.h"
//#include <wx/atomic.h>
//#include <wx/thread.h>

class ODFileDecoder;

/// An AliasBlockFile that references uncompressed data in an existing file
class ODDecodeBlockFile final : public SimpleBlockFile
{
 public:

   // Constructor / Destructor

   /// Create a disk file and write summary and sample data to it
   ODDecodeBlockFile(const QString &baseFileName, const QString &audioFileName, sampleCount aliasStart,
                     size_t aliasLen, int aliasChannel, unsigned int decodeType);
   /// Create the memory structure to refer to the given block file
   ODDecodeBlockFile(const QString &existingFile, const QString &audioFileName, sampleCount aliasStart,
                     size_t aliasLen, int aliasChannel, unsigned int decodeType,
                   float min, float max, float rms, bool dataAvailable);

   virtual ~ODDecodeBlockFile();
   //checks to see if summary data has been computed and written to disk yet.  Thread safe.  Blocks if we are writing summary data.
   bool IsSummaryAvailable() const override;

   /// Returns TRUE if this block's complete data is ready to be accessed by Read()
   bool IsDataAvailable() const override;

   /// Returns TRUE if the summary has not yet been written, but is actively being computed and written to disk
   bool IsSummaryBeingComputed() override { return false; }

   //Calls that rely on summary files need to be overidden
   DiskByteCount GetSpaceUsage() const override;
   /// Gets extreme values for the specified region
   MinMaxRMS GetMinMaxRMS(
      size_t start, size_t len, bool mayThrow) const override;
   /// Gets extreme values for the entire block
   MinMaxRMS GetMinMaxRMS(bool mayThrow) const override;
   /// Returns the 256 byte summary data block
   bool Read256(float *buffer, size_t start, size_t len) override;
   /// Returns the 64K summary data block
   bool Read64K(float *buffer, size_t start, size_t len) override;

   /// returns true before decoding is complete, because it is linked to the encoded file until then.
   /// returns false afterwards.



   ///Makes NEW ODDecodeBlockFile or SimpleBlockFile depending on summary availability
   BlockFilePtr Copy(const QString &fileName) override;

   ///Saves as xml ODDecodeBlockFile or SimpleBlockFile depending on summary availability
   void SaveXML(XMLWriter &xmlFile) override;

   ///Reconstructs from XML a ODDecodeBlockFile and reschedules it for OD loading
   static BlockFilePtr BuildFromXML(DirManager &dm, const QStringMap &attrs);

   ///Writes the summary file if summary data is available
   void Recover(void) override;

   int DoWriteBlockFile(){return WriteODDecodeBlockFile();}

   int WriteODDecodeBlockFile();

   ///Sets the value that indicates where the first sample in this block corresponds to the global sequence/clip.  Only for display use.
   void SetStart(sampleCount startSample){mStart = startSample;}

   ///Gets the value that indicates where the first sample in this block corresponds to the global sequence/clip.  Only for display use.
   sampleCount GetStart() const {return mStart;}

   //returns the number of samples from the beginning of the track that this blockfile starts at
   sampleCount GetGlobalStart() const {return mClipOffset+mStart;}

   //returns the number of samples from the beginning of the track that this blockfile ends at
   sampleCount GetGlobalEnd() const {return mClipOffset+mStart+GetLength();}

   //Below calls are overrided just so we can take wxlog calls out, which are not threadsafe.

   /// Reads the specified data from the aliased file using libsndfile
   size_t ReadData(samplePtr data, sampleFormat format,
                        size_t start, size_t len, bool mayThrow) const override;

   /// Read the summary into a buffer
   bool ReadSummary(ArrayOf<char> &data) override;

   ///Returns the type of audiofile this blockfile is loaded from.
   unsigned int GetDecodeType() /* not override */ const { return mType; }
   // void SetDecodeType(unsigned int type) /* not override */ { mType = type; }

   ///sets the amount of samples the clip associated with this blockfile is offset in the wavetrack (non effecting)
   void SetClipOffset(sampleCount numSamples){mClipOffset= numSamples;}

   ///Gets the number of samples the clip associated with this blockfile is offset by.
   sampleCount GetClipOffset() const {return mClipOffset;}

   //OD TODO:set ISAlias to true while we have no data?

   ///set the decoder,
   void SetODFileDecoder(ODFileDecoder* decoder);

   QString GetAudioFileName(){return mAudioFileName;}

   ///sets the file name the summary info will be saved in.  threadsafe.
   void SetFileName(const QString &name) override;
   QString GetFileName() const override;

   /// Prevents a read on other threads of the encoded audio file.
   void LockRead() const override;
   /// Allows reading of encoded file on other threads.
   void UnlockRead() const override;

   ///// Get the name of the file where the audio data for this block is
   /// stored.
   const QString &GetEncodedAudioFilename()
   {
      return mAudioFileName;
   }

   /// Modify this block to point at a different file.  This is generally
   /// looked down on, but it is necessary in one case: see
   /// DirManager::EnsureSafeFilename().
   void ChangeAudioFile(const QString &newAudioFile);

  protected:

//   void WriteSimpleBlockFile() override;
   //void CalcSummary(samplePtr buffer, size_t len,
   //                 sampleFormat format, ArrayOf<char> &cleanup) override;
   //The on demand type.
   unsigned int mType;

   ///This lock is for the filename (string) of the blockfile that contains summary/audio data
   ///after decoding
   mutable QMutex mFileNameMutex;

   ///The original file the audio came from.
   QString mAudioFileName;

   QAtomicInt mDataAvailable;
   //wxAtomicInt mDataAvailable{ 0 };
   bool mDataBeingComputed;

   ODFileDecoder* mDecoder;
   QMutex mDecoderMutex;

   ///For accessing the audio file that will be decoded.  Used by dir manager;
   mutable QMutex mReadDataMutex;

   ///for reporting after task is complete.  Only for display use.
   sampleCount mStart;

   ///the ODTask needs to know where this blockfile lies in the track, so for convenience, we have this here.
   sampleCount mClipOffset;

   sampleFormat mFormat;

   sampleCount mAliasStart;//where in the encoded audio file this block corresponds to.
   const int   mAliasChannel;//The channel number in the encoded file..

};

#endif

