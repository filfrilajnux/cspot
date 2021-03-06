#include "Player.h"

Player::Player(std::shared_ptr<MercuryManager> manager, std::shared_ptr<AudioSink> audioSink)
{
    this->audioSink = audioSink;
    this->manager = manager;
    
}
void Player::pause()
{
    this->currentTrack->audioStream->isPaused = true;
}

void Player::play()
{
    this->currentTrack->audioStream->isPaused = false;
}

void Player::setVolume(uint16_t volume) {
    this->volume = volume;

    // Pass volume event to the sink if volume is sink-handled
    if (!this->audioSink->softwareVolumeControl) {
        this->audioSink->volumeChanged(volume);
    }
}

void Player::seekMs(size_t positionMs) {
    printf("----- Tryin to seek %d\n", positionMs);
    this->currentTrack->audioStream->seekMs(positionMs);
}

void Player::feedPCM(std::vector<uint8_t> &data) {
    // Simple digital volume control alg
    // @TODO actually extract it somewhere
    if (this->audioSink->softwareVolumeControl) {
	    auto vol = 255-volume;
	    uint32_t value = (log10(255/((float)vol+1)) * 105.54571334);	
	    if (value >= 254) value = 256;
	    auto mult = value<<8; // *256
        int16_t *psample;
	    uint32_t pmax;
	    psample = (int16_t*) (data.data());
	    for (int32_t i = 0; i < (data.size() / 2 ); i++) 
	    {
		    int32_t temp = (int32_t)psample[i] * mult;
		    psample[i] = (temp>>16) & 0xFFFF;	
	    }
    }

    this->audioSink->feedPCMFrames(data);
}

void Player::handleLoad(TrackRef *track, std::function<void()> &trackLoadedCallback)
{
    if (currentTrack != nullptr)
    {
        if (currentTrack->audioStream->isRunning) {
            currentTrack->audioStream->isRunning = false;
            currentTrack->audioStream->waitForTaskToReturn();
        }

        delete currentTrack;
    }

    pcmDataCallback framesCallback = [=](std::vector<uint8_t>& frames) {
        this->feedPCM(frames);
    };

    auto gid = std::vector<uint8_t>(track->gid->bytes, track->gid->bytes + 16);
    currentTrack = new SpotifyTrack(this->manager, gid);
    currentTrack->loadedTrackCallback = [=]() {
        trackLoadedCallback();
        currentTrack->audioStream->streamFinishedCallback = this->endOfFileCallback;
        currentTrack->audioStream->audioSink = audioSink;
        currentTrack->audioStream->pcmCallback = framesCallback;
        currentTrack->audioStream->startTask();
    };
}
