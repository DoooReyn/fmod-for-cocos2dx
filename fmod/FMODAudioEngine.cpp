#include "fmod/FMODAudioEngine.h"
#include "cocos2d.h"

void ERRCHECK_fn(FMOD_RESULT result, const char *file, int line) {
    if (result != FMOD_OK)
    {
        CCLOG("%s(%d): FMOD error %d", file, line, result);
    }
}
#define ERRCHECK(_result) ERRCHECK_fn(_result, __FILE__, __LINE__)

using namespace cocos2d;

static FMODAudioEngine* instance = nullptr;

FMODAudioEngine* FMODAudioEngine::getInstance()
{
    if (instance == nullptr)
    {
        instance = new FMODAudioEngine();
        instance->lazyInit();
    }
    
    return instance;
}

void FMODAudioEngine::destroyInstance()
{
    if (instance)
    {
        delete(instance);
    }
}



FMODAudioEngine::FMODAudioEngine()
:_index(0)
,_musicSid(0)
,_volume(1.0f)
,_music_volume(1.0f)
,_eventListener(0)
,_pitch(1.0f)
,_music_pitch(1.0f)
,_system(NULL)
,_channelGroup(NULL)
,_channelMusicGroup(NULL)
,_channel(NULL)
{
    
}

FMODAudioEngine::~FMODAudioEngine()
{
    releaseAllSounds();
    _system->close();
    _system->release();
}

bool FMODAudioEngine::lazyInit()
{
    FMOD_RESULT       result;
    unsigned int      version;
    void             *extradriverdata = 0;
    
    result = FMOD::System_Create(&_system);
    ERRCHECK(result);
    
    result = _system->init(32, FMOD_INIT_NORMAL, extradriverdata);
    ERRCHECK(result);
    
    result = _system->getVersion(&version);
    ERRCHECK(result);
    
    if (version < FMOD_VERSION)
    {
        CCLOG("FMOD lib version %08x doesn't match header version %08x", version, FMOD_VERSION);
    }
    
    result = _system->createChannelGroup("AllSounds", &_channelGroup);
    ERRCHECK(result);

	result = _system->createChannelGroup("Music", &_channelMusicGroup);
	ERRCHECK(result);
    
    Director::getInstance()->getScheduler()->scheduleUpdate(this, 0, false);
    return true;
}

void FMODAudioEngine::setSoundVolume(float volume)
{
	if (volume == _volume)
	{
		return;
	}
	if (volume > 1.0f) {
		volume = 1.0f;
	}
	if (volume < 0.0f) {
		volume = 0.0f;
	}
	_volume = volume;

	FMOD_RESULT result;
	result = _channelGroup->setVolume(volume);
	ERRCHECK(result);
}

void FMODAudioEngine::setMusicVolume(float volume)
{
	if (volume == _music_volume)
	{
		return;
	}
	if (volume > 1.0f) {
		volume = 1.0f;
	}
	if (volume < 0.0f) {
		volume = 0.0f;
	}
	_music_volume = volume;

	FMOD_RESULT result;
	result = _channelMusicGroup->setVolume(volume);
	ERRCHECK(result);
}
void FMODAudioEngine::update(float fDelta)
{
    FMOD_RESULT result;
    result = _system->update();
    
    auto it = _soundIDMap.begin();
    while (it != _soundIDMap.end())
    {
        FMOD::Channel* channel = it->second;
        bool isPlaying = false;
        result = channel->isPlaying(&isPlaying);
        if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE) && (result != FMOD_ERR_CHANNEL_STOLEN))
        {
            ERRCHECK(result);
        }
        bool paused = false;
        result = channel->getPaused(&paused);
        if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE) && (result != FMOD_ERR_CHANNEL_STOLEN))
        {
            ERRCHECK(result);
        }
        
        if (!paused && !isPlaying)
        {
            int nSoundID = it->first;
            this->onEvent(FMODAudioEventType::STOP, nSoundID);
            _soundIDMap.erase(it++);
			if (_musicSid == nSoundID) {
				_musicSid = 0;
			}
        }
        else
        {
            ++it;
        }
    }
}

void FMODAudioEngine::releaseSound(const std::string &filename)
{
    std::string key = FileUtils::getInstance()->fullPathForFilename(filename);
    auto it = _sounds.find(key);
    FMOD::Sound* sound = NULL;
    if (it != _sounds.end())
    {
        sound = it->second;
        FMOD_RESULT result = sound->release();
        ERRCHECK(result);
    }
    else
    {
        CCLOG("can't find the sound %s", filename.c_str());
    }
}

void FMODAudioEngine::releaseMusic(const std::string &filename) {
	if (isMusicPlaying()) {
		_musicSid = 0;
		releaseSound(filename);
	}
}

void FMODAudioEngine::releaseAllSounds()
{
    FMOD_RESULT result;
    for (auto& s : _sounds)
    {
        FMOD::Sound* sound = s.second;

        result = sound->release();
        ERRCHECK(result);
    }
    _sounds.clear();

	_musicSid = 0;
}

FMOD::Sound* FMODAudioEngine::load(const std::string &filename)
{
    FMOD_RESULT result;
    FMOD::Sound *sound;
    std::string key = FileUtils::getInstance()->fullPathForFilename(filename);
    if (!FileUtils::getInstance()->isFileExist(key))
    {
        CCLOG("FMODAudioEngine preload file not found %s", filename.c_str());
        return NULL;
    }
    result = _system->createSound(key.c_str(), FMOD_DEFAULT, 0, &sound);
    ERRCHECK(result);
    return sound;
}

FMOD::Sound* FMODAudioEngine::getOrCreateIfNotExists(const std::string &filename, bool isMusic/* = false*/)
{
    std::string key = FileUtils::getInstance()->fullPathForFilename(filename);
    auto it = _sounds.find(key);
    FMOD::Sound* sound = NULL;
    if (it != _sounds.end())
    {
        sound = it->second;
    }
    if (sound == NULL)
    {
        sound = load(filename);
        if (sound)
        {
			if (isMusic) {
				//sound->setUserData((void*)_musicSymbol);
				sound->setUserData(_channelMusicGroup);
			}
            _sounds.emplace(key, sound);
        }
    }
    
    return sound;
}

void FMODAudioEngine::preload(const std::string &filename)
{
    getOrCreateIfNotExists(filename, false);
}

void FMODAudioEngine::preloadMusic(const std::string &filename)
{
	getOrCreateIfNotExists(filename, true);
}

bool FMODAudioEngine::isMusicPlaying()
{
	if (_musicSid <= 0) return false;

	FMOD::Channel* channel = NULL;
	auto it = _soundIDMap.find(_musicSid);
	FMOD_RESULT result;
	if (it != _soundIDMap.end()) {
		channel = it->second;
		bool paused = false;
		result = channel->getPaused(&paused);
		ERRCHECK(result);
		return paused;
	}

	return false;
}

void FMODAudioEngine::rewindMusic()
{
	if (_musicSid <= 0) return;

	FMOD::Channel* channel = NULL;
	auto it = _soundIDMap.find(_musicSid);
	FMOD_RESULT result;
	if (it != _soundIDMap.end()) {
		channel = it->second;
		result = channel->setPosition(0, FMOD_TIMEUNIT_MS);
		ERRCHECK(result);
	}
}

int FMODAudioEngine::playMusic(const std::string &filename, bool loop /*= false */)
{
	if (_musicSid > 0) {
		stopMusic();
	}

	FMOD_RESULT result;
	int nSoundID = genSoundID();

	FMOD::Sound* sound = getOrCreateIfNotExists(filename, true);

	if (sound == nullptr)
	{
		return  -1;
	}

	FMOD::Channel* channel = nullptr;

	result = sound->setMode(loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
	ERRCHECK(result);

	if (loop)
	{
		result = sound->setLoopCount(-1);
		ERRCHECK(result);
	}
	else
	{
		result = sound->setLoopCount(0);
		ERRCHECK(result);
	}

	result = _system->playSound(sound, _channelMusicGroup, false, &channel);
	ERRCHECK(result);

	_soundIDMap.emplace(nSoundID, channel);
	_musicSid = nSoundID;
	return nSoundID;
}

int FMODAudioEngine::playSound(const std::string &filename, bool loop /*= false */)
{
    FMOD_RESULT result;
    int nSoundID = genSoundID();
    
    FMOD::Sound* sound = getOrCreateIfNotExists(filename, false);
    
    if (sound == nullptr)
    {
        return  -1;
    }
    
    FMOD::Channel* channel = nullptr;
    
    result = sound->setMode(loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
    ERRCHECK(result);
    
    if (loop)
    {
        result = sound->setLoopCount(-1);
        ERRCHECK(result);
    }
    else
    {
        result = sound->setLoopCount(0);
        ERRCHECK(result);
    }
    
    result = _system->playSound(sound, _channelGroup, false, &channel);
    ERRCHECK(result);
    
    _soundIDMap.emplace(nSoundID, channel);
    return nSoundID;
}

void FMODAudioEngine::pauseSound(int soundID)
{
    FMOD::Channel* channel = NULL;
    auto it = _soundIDMap.find(soundID);
    FMOD_RESULT result;
    if (it != _soundIDMap.end()) {
        channel = it->second;
        result = channel->setPaused(true);
        ERRCHECK(result);
    }
    else
    {
        CCLOG("unable find sound info by id = %d", soundID);
    }
}

void FMODAudioEngine::pauseMusic()
{
	if (_musicSid > 0) {
		pauseSound(_musicSid);
	}
}

void FMODAudioEngine::resumeSound(int soundID)
{
    FMOD::Channel* channel = NULL;
    auto it = _soundIDMap.find(soundID);
    FMOD_RESULT result;
    if (it != _soundIDMap.end()) {
        channel = it->second;
        result = channel->setPaused(false);
        ERRCHECK(result);
    }
    else
    {
        CCLOG("unable find sound info by id = %d", soundID);
    }
}

void FMODAudioEngine::resumeMusic()
{
	if (_musicSid > 0) {
		resumeSound(_musicSid);
	}
}

void FMODAudioEngine::pauseAllSounds(bool with_music /*= false */)
{
    FMOD_RESULT result;
    result = _channelGroup->setPaused(true);
    ERRCHECK(result);
	if (with_music) {
		result = _channelMusicGroup->setPaused(true);
		ERRCHECK(result);
	}
}

void FMODAudioEngine::resumeAllSounds(bool with_music)
{
    FMOD_RESULT result;
    result = _channelGroup->setPaused(false);
    ERRCHECK(result);
	if (with_music) {
		result = _channelMusicGroup->setPaused(false);
		ERRCHECK(result);
	}
}

void FMODAudioEngine::stopMusic()
{
	if (_musicSid <= 0) return;

	FMOD_RESULT result;
	result = _channelMusicGroup->stop();
	ERRCHECK(result);

	//FMOD::Channel* channel = NULL;
	//auto it = _soundIDMap.find(_musicSid);
	//FMOD_RESULT result;
	//if (it != _soundIDMap.end()) {
	//	channel = it->second;
	//	result = channel->stop();
	//	_musicSid = 0;
	//	ERRCHECK(result);
	//}
	//else
	//{
	//	CCLOG("unable find sound info by id = %d", _musicSid);
	//}
}

void FMODAudioEngine::stopSound(int soundID)
{
    FMOD::Channel* channel = NULL;
    auto it = _soundIDMap.find(soundID);
    FMOD_RESULT result;
    if (it != _soundIDMap.end()) {
        channel = it->second;
        result = channel->stop();
        ERRCHECK(result);
    }
    else
    {
        CCLOG("unable find sound info by id = %d", soundID);
    }
}

void FMODAudioEngine::stopAllSounds(bool with_music)
{
    FMOD_RESULT result;
    result = _channelGroup->stop();
    ERRCHECK(result);
	if (with_music) {
		result = _channelMusicGroup->stop();
		ERRCHECK(result);
	}
}

void FMODAudioEngine::setPitch(float pitch)
{
    if (pitch == _pitch)
    {
        return;
    }
    _pitch = pitch;
    
    FMOD_RESULT result;
    result = _channelGroup->setPitch(pitch);
    ERRCHECK(result);
}

void FMODAudioEngine::setMusicPitch(float pitch) {
	if (pitch == _music_pitch)
	{
		return;
	}
	_music_pitch = pitch;

	FMOD_RESULT result;
	result = _channelMusicGroup->setPitch(pitch);
	ERRCHECK(result);
}

int FMODAudioEngine::genSoundID()
{
    int nID = -1;
    if (_availableIDs.size() > 0)
    {
        nID = _availableIDs.at(0);
        _availableIDs.erase(_availableIDs.begin());
        return nID;
    }
    
    _index++;
    nID = _index;
    return nID;
}

void FMODAudioEngine::addEventListener(int listener)
{
    if (_eventListener != 0)
    {
        ScriptEngineManager::getInstance()->getScriptEngine()->removeScriptHandler(_eventListener);
    }
    _eventListener = listener;
}

void FMODAudioEngine::onEvent(FMODAudioEventType eventType, int soundID)
{
    if (_eventListener != 0)
    {
//        LuaStack *luaStack = LuaEngine::getInstance()->getLuaStack();
//        lua_State *L = luaStack->getLuaState();
//        luaStack->pushInt(eventType);
//        luaStack->pushInt(soundID);
//        LuaEngine::getInstance()->getLuaStack()->executeFunctionByHandler(_eventListener, 2);
        CommonScriptData data(_eventListener, StringUtils::format("stop_%d", soundID).c_str());
        ScriptEvent event(kCommonEvent, (void*)&data);
        ScriptEngineProtocol* engine = ScriptEngineManager::getInstance()->getScriptEngine();
        engine->sendEvent(&event);
    }
}
