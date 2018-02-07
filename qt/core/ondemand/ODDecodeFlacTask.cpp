/*
 *  ODDecodeFlacTask.cpp
 *  Audacity
 *
 *  Created by apple on 8/10/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

//--#include "../Audacity.h"
#include "ODDecodeFlacTask.h"

//--#include "../Prefs.h"
//--#include <wx/string.h>
//--#include <wx/utils.h>
//--#include <wx/file.h>
//--#include <wx/ffile.h>

#include <unistd.h> // dup() close()

//#ifdef USE_LIBID3TAG
//extern "C" {
//#include <id3tag.h>
//}
//#endif

//#include "../Tags.h"

#define FLAC_HEADER "fLaC"

#define DESC _("FLAC files")

ODDecodeFlacTask::~ODDecodeFlacTask()
{
}

movable_ptr<ODTask> ODDecodeFlacTask::Clone() const
{
   auto clone = make_movable<ODDecodeFlacTask>();
   clone->mDemandSample = GetDemandSample();

   //the decoders and blockfiles should not be copied.  They are created as the task runs.
   // This std::move is needed to "upcast" the pointer type
   return std::move(clone);
}

void ODFLACFile::metadata_callback(const FLAC__StreamMetadata *metadata)
{
   switch (metadata->type)
   {
      case FLAC__METADATA_TYPE_VORBIS_COMMENT:
         for (FLAC__uint32 i = 0; i < metadata->data.vorbis_comment.num_comments; i++) {
            mComments.append(QString::fromUtf8((const char*)metadata->data.vorbis_comment.comments[i].entry));
         }
      break;

      case FLAC__METADATA_TYPE_STREAMINFO: {
//         mDecoder->mSampleRate=metadata->data.stream_info.sample_rate;
           mDecoder->mNumChannels=metadata->data.stream_info.channels;
//         mDecoder->mBitsPerSample=metadata->data.stream_info.bits_per_sample;
//         mDecoder->mNumSamples=metadata->data.stream_info.total_samples;

         auto bitsPerSample = metadata->data.stream_info.bits_per_sample;

         // ???? Will we see this metatdata multiple times with different formats?
         if (bitsPerSample <= 16) {
            if (mDecoder->mFormat < int16Sample)
               mDecoder->mFormat=int16Sample;
         } else if (bitsPerSample <= 24) {
            if (mDecoder->mFormat < int24Sample)
               mDecoder->mFormat = int24Sample;
         } else {
            // flac spec supports up to 32 bits per sample, convert it to float sample
            mDecoder->mFormat=floatSample;
         }

         //mDecoder->mStreamInfoDone=true;
      }
      break;
      // handle the other types we do nothing with to avoid a warning
      case FLAC__METADATA_TYPE_PADDING:	// do nothing with padding
      case FLAC__METADATA_TYPE_APPLICATION:	// no idea what to do with this
      case FLAC__METADATA_TYPE_SEEKTABLE:	// don't need a seektable here
      case FLAC__METADATA_TYPE_CUESHEET:	// convert this to labels?
      case FLAC__METADATA_TYPE_PICTURE:		// ignore pictures
      case FLAC__METADATA_TYPE_UNDEFINED:	// do nothing with this either
      case FLAC__MAX_METADATA_TYPE:

      break;
   }
}

void ODFLACFile::error_callback(FLAC__StreamDecoderErrorStatus status)
{
   mWasError = true;

   switch (status)
   {
   case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC:
      qWarning("Flac Error: Lost sync");
      break;
   case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH:
      qWarning("Flac Error: Crc mismatch");
      break;
   case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER:
      qWarning("Flac Error: Bad Header");
      break;
   default:
      qWarning("Flac Error: Unknown error code");
      break;
   }
}

//the inside of the read loop.
FLAC__StreamDecoderWriteStatus ODFLACFile::write_callback(const FLAC__Frame *frame,
                       const FLAC__int32 * const buffer[])
{
   size_t samplesToCopy = frame->header.blocksize;
   const size_t samplesAvailable = mDecoder->mDecodeBufferLen - mDecoder->mDecodeBufferWritePosition;
   if(samplesToCopy > samplesAvailable)
   {
      qWarning("FLAC decode ran out of buffer space");
      samplesToCopy = samplesAvailable;
   }

   //the decodeBuffer was allocated to be the same format as the flac buffer, so we can do a straight up memcpy.

   // BUG: this memcpy is illegal because it tries to copy over an array of int32* to an array of int16*
//   memcpy(mDecoder->mDecodeBuffer + SAMPLE_SIZE(mDecoder->mFormat)*mDecoder->mDecodeBufferWritePosition,
//          buffer[mDecoder->mTargetChannel],
//          SAMPLE_SIZE(mDecoder->mFormat)*samplesToCopy);

   const FLAC__int32* src = buffer[mDecoder->mTargetChannel];

   if (mDecoder->mFormat == int16Sample)
   {
      FLAC__int16* dst = (FLAC__int16*)mDecoder->mDecodeBuffer + mDecoder->mDecodeBufferWritePosition;
      // note: cannot memcpy() since the int32 must cast to int16
      for (unsigned int i = 0; i < samplesToCopy; i++)
         dst[i] = src[i];
   }
   else
   if (mDecoder->mFormat == int24Sample)
   {
      // memcpy might be OK for 24-bit...
      static_assert(sizeof(*src) == SAMPLE_SIZE(int24Sample), "Unable to memcpy the 24-bit case");

      void* dst = mDecoder->mDecodeBuffer + SAMPLE_SIZE(mDecoder->mFormat)*mDecoder->mDecodeBufferWritePosition;
      memcpy(dst, src, SAMPLE_SIZE(int24Sample)*samplesToCopy);
   }
   else
   {
      // convert 32bit -> float
      qFatal("Unsupported format");
   }

   //for (int i = 0; i < samplesToCopy; i++)
   //   qDebug("%d", ((short*)(mDecoder->mDecodeBuffer + SAMPLE_SIZE(mDecoder->mFormat)*mDecoder->mDecodeBufferWritePosition))[i]);

   mDecoder->mDecodeBufferWritePosition += samplesToCopy;


/*
   ArrayOf<short> tmp{ frame->header.blocksize };

   for (unsigned int chn=0; chn<mDecoder->mNumChannels; chn++) {
      if (frame->header.bits_per_sample == 16) {
         for (unsigned int s=0; s<frame->header.blocksize; s++) {
            tmp[s]=buffer[chn][s];
         }

         mDecoder->mChannels[chn]->Append((samplePtr)tmp.get(),
                  int16Sample,
                  frame->header.blocksize);
      }
      else {
         mDecoder->mChannels[chn]->Append((samplePtr)buffer[chn],
                  int24Sample,
                  frame->header.blocksize);
      }
   }
*/

//   mDecoder->mSamplesDone += frame->header.blocksize;

   return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
//   return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

//   mDecoder->mUpdateResult = mDecoder->mProgress->Update((wxULongLong_t) mDecoder->mSamplesDone, mDecoder->mNumSamples != 0 ? (wxULongLong_t)mDecoder->mNumSamples : 1);
/*
   if (mDecoder->mUpdateResult != ProgressResult::Success)
   {
      return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
   }

   return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
   */
}


//--Decoder stuff:
   ///Decodes the samples for this blockfile from the real file into a float buffer.
   ///This is file specific, so subclasses must implement this only.
   ///the buffer was defined like
   ///samplePtr sampleData = NewSamples(mLen, floatSample);
   ///this->ReadData(sampleData, floatSample, 0, mLen);
   ///This class should call ReadHeader() first, so it knows the length, and can prepare
   ///the file object if it needs to.
int ODFlacDecoder::Decode(SampleBuffer & data, sampleFormat & format, sampleCount start, sampleCount len, unsigned int channel)
{

   //we need to lock this so the target stays fixed over the seek/write callback.
//   mFlacFileLock.lock();

//   bool usingCache=mLastDecodeStartSample==start;
//   if(usingCache)
//   {
//      //we've just decoded this, so lets use a cache.  (often so for
//   }

   mDecodeBufferWritePosition = 0;
   mDecodeBufferLen = len.as_size_t();
   mTargetChannel = channel;

   data.Allocate(mDecodeBufferLen, mFormat);
   mDecodeBuffer = data.ptr();

   format = mFormat;

   // FIXME: check that start+len is a valid range

   // If anything bad should happen, return cleared buffer
   ClearSamples(mDecodeBuffer, mFormat, start.as_size_t(), mDecodeBufferLen);

   // we don't support 32-bit FLAC
   if (mFormat == floatSample || channel >= mNumChannels)
   {
      qWarning("ODFlacDecoder::Decode: Unsupported FLAC format or channel count");
      return -1;
   }

   // We load/unload the flac decoder each time Decode() is called
   // - seek_absolute() will not work on some platforms when the file has been read out (win32)
   // - TESTME: this might not require any locking to guard FLAC library (if functions are reentrant!)
   ODFLACFile flac(this);
   if (flac.init(mFName.toUtf8().data()) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
   {
      qWarning("ODFlacDecoder::Decode: init() failed");
      return -2;
   }

   // Third party library has its own type alias, check it
   static_assert(sizeof(sampleCount::type) <= sizeof(FLAC__int64),
                 "Type FLAC__int64 is too narrow to hold a sampleCount");

   // FIXME: seek_absolute() fails for some reason
   // something is wrong with library usage...
   if (start > 0 && !flac.seek_absolute(static_cast<FLAC__int64>( start.as_long_long() )))
   {
      qWarning("ODFlacDecoder::Decode: seek_absolute failed");
      return -3;
   }

   while (mDecodeBufferWritePosition < mDecodeBufferLen)
      flac.process_single(); // TODO: error check

//   mFlacFileLock.unlock();
//   if(!usingCache)
//   {
//      mLastDecodeStartSample=start;
//   }

   //qDebug("ODFlacDecoder: decoded %d of %d samples from start=%d", (int)mDecodeBufferWritePosition,
   //       (int)len, (int)start.as_long_long());

   //insert into blockfile and
   //calculate summary happen in ODDecodeBlockFile::WriteODDecodeBlockFile, where this method is also called.
   return 1;
}

///Read header.  Subclasses must override.  Probably should save the info somewhere.
///Ideally called once per decoding of a file.  This complicates the task because
///returns true if the file exists and the header was read alright.

//Note:we are not using LEGACY_FLAC defs (see ImportFlac.cpp FlacImportFileHandle::Init()
//this code is based on that function.
bool ODFlacDecoder::ReadHeader()
{
   mFormat = int16Sample;//start with the smallest and move up in the metadata_callback.
                         //we want to use the native flac type for quick conversion.
      /* (sampleFormat)
      gPrefs->Read(wxT("/SamplingRate/DefaultProjectSampleFormat"), floatSample);*/
//   mFile = std::make_unique<ODFLACFile>(this);


   // old versions did not support unicode file names on Windows
#if FLAC_API_VERSION_CURRENT < 11
#error Need at least flac verson 1.3.1 for unicode support
#endif
   ODFLACFile flac(this);

   int result = flac.init(mFName.toUtf8().data());
   if (result != FLAC__STREAM_DECODER_INIT_STATUS_OK)
      return false;

   //this will call the metadata_callback when it is done
   flac.process_until_end_of_metadata();
   // not necessary to check state, error callback will catch errors, but here's how:
   if (flac.get_state() > FLAC__STREAM_DECODER_READ_FRAME)
      return false;

   if (!flac.is_valid() || flac.get_was_error()) {
      // This probably is not a FLAC file at all
      return false;
   }

   MarkInitialized();
   return true;
}

//ODFLACFile* ODFlacDecoder::GetFlacFile()
//{
//   return mFile.get();
//}

ODFlacDecoder::ODFlacDecoder(const QString & fileName)
   : ODFileDecoder(fileName),
     mFormat((sampleFormat)0),
//     mFile(nullptr),
     mNumChannels(0),
     mTargetChannel(0),
     mDecodeBufferWritePosition(0),
     mDecodeBufferLen(0)
{
}

ODFlacDecoder::~ODFlacDecoder(){
//   qDebug("destruct");
//   if(mFile)
//      mFile->finish();
}

///Creates an ODFileDecoder that decodes a file of filetype the subclass handles.
//
//compare to FLACImportPlugin::Open(wxString filename)
ODFileDecoder* ODDecodeFlacTask::CreateFileDecoder(const QString & fileName)
{
   // First check if it really is a FLAC file
/*
   int cnt;
   wxFile binaryFile;
   if (!binaryFile.Open(fileName)) {
      return NULL; // File not found
   }

#ifdef USE_LIBID3TAG
   // Skip any ID3 tags that might be present
   id3_byte_t query[ID3_TAG_QUERYSIZE];
   cnt = binaryFile.Read(query, sizeof(query));
   cnt = id3_tag_query(query, cnt);
   binaryFile.Seek(cnt);
#endif

   char buf[5];
   cnt = binaryFile.Read(buf, 4);
   binaryFile.Close();

   if (cnt == wxInvalidOffset || strncmp(buf, FLAC_HEADER, 4) != 0) {
      // File is not a FLAC file
      return NULL;
   }

   // Open the file for import
   auto decoder = std::make_movable<ODFlacDecoder>(fileName);
*/
/*
   bool success = decoder->Init();
   if (!success) {
      return NULL;
   }
*/
   // Open the file for import
   auto decoder = make_movable<ODFlacDecoder>(fileName);

   mDecoders.push_back(std::move(decoder));
   return mDecoders.back().get();
}
