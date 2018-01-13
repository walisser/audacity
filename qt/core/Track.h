#ifndef __AUDACITY_TRACK__
#define __AUDACITY_TRACK__

class Track {

public:
   enum
   {
      LeftChannel = 0,
      RightChannel = 1,
      MonoChannel = 2
   };

   enum TrackKindEnum
   {
      None,
      Wave,
#if defined(USE_MIDI)
      Note,
#endif
      Label,
      Time,
      All
   };
};


#endif
