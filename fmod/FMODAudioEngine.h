#ifndef FMODAUDIOENGINE_H
#define FMODAUDIOENGINE_H
#include "fmod/include/win32/fmod.hpp"
#include <functional>
#include <list>
#include <string>
#include <unordered_map>
#include <vector> 
#include "base/CCRef.h"
enum FMODAudioEventType
{
    STOP
};
class CC_DLL FMODAudioEngine : public cocos2d::Ref
{
public:
    FMODAudioEngine();
    ~FMODAudioEngine();
    
    static FMODAudioEngine * getInstance();
    static void destroyInstance();
    
    bool lazyInit();
    
    FMOD::Sound* getOrCreateIfNotExists(const std::string &filename, bool isMusic=false);
    
    FMOD::Sound* load(const std::string &filename);
    
    void preload(const std::string &filename);
	void preloadMusic(const std::string &filename);
    
    int playSound(const std::string &filename, bool loop = false);
	int playMusic(const std::string &filename, bool loop = false);
	bool isMusicPlaying();

    void pauseSound(int soundID);
	void pauseMusic();
	void pauseAllSounds(bool with_music = false);
	int getMusicId() { return _musicSid; };

	void rewindMusic();
    
    void resumeSound(int soundID);
	void resumeMusic();
	void resumeAllSounds(bool with_music = false);
    
    void stopSound(int soundID);
	void stopMusic();
    void stopAllSounds(bool with_music = false);
    
    void setPitch(float pitch);
	void setMusicPitch(float pitch);
	void setSoundVolume(float volume);
	void setMusicVolume(float volume);

    float getPitch() {return _pitch;}
	float getMusicPitch() { return _music_pitch; }
	float getMusicVolume() { return _music_volume;  }
	float getSoundVolume() { return _volume;  }

    void releaseSound(const std::string &filename);
	void releaseMusic(const std::string &filename);
	void releaseAllSounds();
    
    void addEventListener(int listener);
    
    void onEvent(FMODAudioEventType eventType, int soundID);
    
    int genSoundID();
    
    virtual void update(float delta);
public:
    FMOD::System* _system;
    FMOD::ChannelGroup* _channelGroup;
	FMOD::ChannelGroup* _channelMusicGroup;
    FMOD::Channel* _channel;
	const char* _musicSymbol = "1";
    
private:
    int _index;
    int _eventListener;
	int _musicSid;
    std::unordered_map<std::string, FMOD::Sound*> _sounds;
    std::unordered_map<int, FMOD::Channel*> _soundIDMap;
    std::vector<int> _availableIDs;
    
    float _pitch;
	float _music_pitch;
	float _music_volume;
	float _volume;
};

#endif
