#pragma once
enum enumSoundFormat
{
	AUDIO_U8 = 0x0008,	/**< Unsigned 8-bit samples */
	AUDIO_S8 = 0x8008,	/**< Signed 8-bit samples */
	AUDIO_U16LSB = 0x0010,	/**< Unsigned 16-bit samples */
	AUDIO_S16LSB = 0x8010,	/**< Signed 16-bit samples */
	AUDIO_U16MSB = 0x1010,	/**< As above, but big-endian byte order */
	AUDIO_S16MSB = 0x9010	/**< As above, but big-endian byte order */
};
class CPlaySound
{
public:
	
	//MUST be invoked once and only once!
	//supported parameters: channels = 1 or 2
	//create a background thread (to playsound)
	//init stat: PAUSE
	static bool Init(enumSoundFormat format = AUDIO_U8, int channels = 1, int sampling_rate = 8000, int max_packet_size = 1000);

	//set stat = PLAY
	static bool Play();

	//set stat = PAUSE
	static bool Pause();

	//clear all data
	static bool Clear();

	//wait until all data are consumed
	static bool Wait();

	//push data into queue
	//wait if pkt size >= max pkt size
	static bool Push(char* buf, int len);	

	//close the thread
	static bool Release();
};