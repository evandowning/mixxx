/***************************************************************************
                          enginebuffer.cpp  -  description
                             -------------------
    begin                : Wed Feb 20 2002
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "enginebuffer.h"

#include <qevent.h>
#include "configobject.h"
#include "controlpushbutton.h"
#include "controlpotmeter.h"
#include "controlttrotary.h"
#include "controlengine.h"
#include "controlbeat.h"
#include "reader.h"
#include "readerextractbeat.h"
#include "enginebufferscalesrc.h"
#include "powermate.h"
#include "wvisualwaveform.h"
#include "visual/visualchannel.h"
#include "mathstuff.h"


// Static default values for rate buttons
double EngineBuffer::m_dTemp = 0.01;
double EngineBuffer::m_dTempSmall = 0.001;
double EngineBuffer::m_dPerm = 0.01;
double EngineBuffer::m_dPermSmall = 0.001;

EngineBuffer::EngineBuffer(PowerMate *_powermate, const char *_group)
{
    group = _group;
    powermate = _powermate;

    m_pOtherEngineBuffer = 0;

    m_bTempPress = false;

    // Play button
    ControlPushButton *p = new ControlPushButton(ConfigKey(group, "play"), true);
    playButton = new ControlEngine(p);
    connect(playButton, SIGNAL(valueChanged(double)), this, SLOT(slotControlPlay(double)));
    playButton->set(0);

    // Reverse button
    p = new ControlPushButton(ConfigKey(group, "reverse"));
    reverseButton = new ControlEngine(p);
    reverseButton->set(0);

    // Fwd button
    p = new ControlPushButton(ConfigKey(group, "fwd"));
    fwdButton = new ControlEngine(p);
    fwdButton->set(0);

    // Back button
    p = new ControlPushButton(ConfigKey(group, "back"));
    backButton = new ControlEngine(p);
    backButton->set(0);

    // Start button
    p = new ControlPushButton(ConfigKey(group, "start"));
    startButton = new ControlEngine(p);
    connect(startButton, SIGNAL(valueChanged(double)), this, SLOT(slotControlStart(double)));
    startButton->set(0);

    // End button
    p = new ControlPushButton(ConfigKey(group, "end"));
    endButton = new ControlEngine(p);
    connect(endButton, SIGNAL(valueChanged(double)), this, SLOT(slotControlEnd(double)));
    endButton->set(0);

    // Cue set button:
    p = new ControlPushButton(ConfigKey(group, "cue_set"));
    buttonCueSet = new ControlEngine(p);
    connect(buttonCueSet, SIGNAL(valueChanged(double)), this, SLOT(slotControlCueSet(double)));

    // Cue goto button:
    p = new ControlPushButton(ConfigKey(group, "cue_goto"));
    buttonCueGoto = new ControlEngine(p);
    connect(buttonCueGoto, SIGNAL(valueChanged(double)), this, SLOT(slotControlCueGoto(double)));

    // Cue point
    ControlObject *p5 = new ControlObject(ConfigKey(group, "cue_point"));
    cuePoint = new ControlEngine(p5);
	    
    // Cue preview button:
    p = new ControlPushButton(ConfigKey(group, "cue_preview"));
    buttonCuePreview = new ControlEngine(p);
    connect(buttonCuePreview, SIGNAL(valueChanged(double)), this, SLOT(slotControlCuePreview(double)));

    // Playback rate slider
    ControlPotmeter *p2 = new ControlPotmeter(ConfigKey(group, "rate"), 0.9f, 1.1f);
    rateSlider = new ControlEngine(p2);

    // Rate display
    p5 = new ControlObject(ConfigKey(group, "rateDisplay"));
    ControlObject::connectControls(ConfigKey(group, "rate"), ConfigKey(group, "rateDisplay"));

    // Permanent rate-change buttons
    p = new ControlPushButton(ConfigKey(group,"rate_perm_down"));
    buttonRatePermDown = new ControlEngine(p);
    connect(buttonRatePermDown, SIGNAL(valueChanged(double)), this, SLOT(slotControlRatePermDown(double)));
    p = new ControlPushButton(ConfigKey(group,"rate_perm_down_small"));
    buttonRatePermDownSmall = new ControlEngine(p);
    connect(buttonRatePermDownSmall, SIGNAL(valueChanged(double)), this, SLOT(slotControlRatePermDownSmall(double)));
    p = new ControlPushButton(ConfigKey(group,"rate_perm_up"));
    buttonRatePermUp = new ControlEngine(p);
    connect(buttonRatePermUp, SIGNAL(valueChanged(double)), this, SLOT(slotControlRatePermUp(double)));
    p = new ControlPushButton(ConfigKey(group,"rate_perm_up_small"));
    buttonRatePermUpSmall = new ControlEngine(p);
    connect(buttonRatePermUpSmall, SIGNAL(valueChanged(double)), this, SLOT(slotControlRatePermUpSmall(double)));

    // Temporary rate-change buttons
    p = new ControlPushButton(ConfigKey(group,"rate_temp_down"));
    buttonRateTempDown = new ControlEngine(p);
    connect(buttonRateTempDown, SIGNAL(valueChanged(double)), this, SLOT(slotControlRateTempDown(double)));
    p = new ControlPushButton(ConfigKey(group,"rate_temp_down_small"));
    buttonRateTempDownSmall = new ControlEngine(p);
    connect(buttonRateTempDownSmall, SIGNAL(valueChanged(double)), this, SLOT(slotControlRateTempDownSmall(double)));
    p = new ControlPushButton(ConfigKey(group,"rate_temp_up"));
    buttonRateTempUp = new ControlEngine(p);
    connect(buttonRateTempUp, SIGNAL(valueChanged(double)), this, SLOT(slotControlRateTempUp(double)));
    p = new ControlPushButton(ConfigKey(group,"rate_temp_up_small"));
    buttonRateTempUpSmall = new ControlEngine(p);
    connect(buttonRateTempUpSmall, SIGNAL(valueChanged(double)), this, SLOT(slotControlRateTempUpSmall(double)));

    // Wheel to control playback position/speed
    ControlTTRotary *p3 = new ControlTTRotary(ConfigKey(group, "wheel"));
    wheel = new ControlEngine(p3);

    // Slider to show and change song position
    ControlPotmeter *controlplaypos = new ControlPotmeter(ConfigKey(group, "playposition"), 0., 1.);
    playposSlider = new ControlEngine(controlplaypos);
    connect(playposSlider, SIGNAL(valueChanged(double)), this, SLOT(slotControlSeek(double)));

    // Potmeter used to communicate bufferpos_play to GUI thread
    ControlPotmeter *controlbufferpos = new ControlPotmeter(ConfigKey(group, "bufferplayposition"), 0., READBUFFERSIZE);
    bufferposSlider = new ControlEngine(controlbufferpos);

    // Control used to communicate absolute playpos to GUI thread
    ControlObject *controlabsplaypos = new ControlObject(ConfigKey(group, "absplayposition"));
    absPlaypos = new ControlEngine(controlabsplaypos);

    // m_pTrackEnd is used to signal when at end of file during playback
    p5 = new ControlObject(ConfigKey(group, "TrackEnd"));
    m_pTrackEnd = new ControlEngine(p5);

    // Direction of rate slider
    p5 = new ControlObject(ConfigKey(group, "rate_dir"));
    m_pRateDir = new ControlEngine(p5);

    // TrackEndMode determines what to do at the end of a track
    p5 = new ControlObject(ConfigKey(group,"TrackEndMode"));
    m_pTrackEndMode = new ControlEngine(p5);

    // BPM control
    ControlBeat *p4 = new ControlBeat(ConfigKey(group, "bpm"), true);
    bpmControl = new ControlEngine(p4);
    connect(bpmControl, SIGNAL(valueChanged(double)), this, SLOT(slotSetBpm(double)));

    // Beat event control
    p2 = new ControlPotmeter(ConfigKey(group, "beatevent"));
    beatEventControl = new ControlEngine(p2);

    // Beat sync (scale buffer tempo relative to tempo of other buffer)
    p = new ControlPushButton(ConfigKey(group, "beatsync"));
    buttonBeatSync = new ControlEngine(p);
    connect(buttonBeatSync, SIGNAL(valueChanged(double)), this, SLOT(slotControlBeatSync(double)));

    // Audio beat mark toggle
    p = new ControlPushButton(ConfigKey(group, "audiobeatmarks"));
    audioBeatMark = new ControlEngine(p);

    // Control file changed
//    filechanged = new ControlEngine(controlfilechanged);
//    filechanged->setNotify(this,(EngineMethod)&EngineBuffer::newtrack);

    m_bCuePreview = false;

    setNewPlaypos(0.);

    reader = new Reader(this, &rate_exchange, &pause);
    read_buffer_prt = reader->getBufferWavePtr();
    file_length_old = -1;
    file_srate_old = 0;
    rate_old = 0;

    m_iBeatMarkSamplesLeft = 0;

    // Allocate buffer for processing:
    buffer = new CSAMPLE[MAX_BUFFER_LEN];

    // Construct scaling object
    scale = new EngineBufferScaleSRC(reader->getWavePtr());

    oldEvent = 0.;

    // Used in update of playpos slider
    m_iSamplesCalculated = 0;


    reader->start();
}

EngineBuffer::~EngineBuffer()
{
    delete playButton;
    delete wheel;
    delete rateSlider;
    delete [] buffer;
    delete scale;
    delete bufferposSlider;
    delete absPlaypos;
    delete m_pTrackEnd;
    delete reader;
}

void EngineBuffer::setQuality(int q)
{
    // Change sound interpolation quality
    scale->setQuality(q);
}

void EngineBuffer::setVisual(WVisualWaveform *pVisualWaveform)
{
    VisualChannel *pVisualChannel = 0;
    // Try setting up visuals
    if (pVisualWaveform)
    {
        // Add buffer as a visual channel
        pVisualChannel = pVisualWaveform->add((ControlPotmeter *)ControlObject::getControl(ConfigKey(group, "bufferplayposition")), group);
        reader->addVisual(pVisualChannel);
    }
}

float EngineBuffer::getDistanceNextBeatMark()
{
    float *p = (float *)reader->getBeatPtr()->getBasePtr();
    int size = reader->getBeatPtr()->getBufferSize();

    int pos = (int)(bufferpos_play*size/READBUFFERSIZE);

    qDebug("s1 %i s2 %i",size,READBUFFERSIZE);

    int i;
    bool found = false;
    for (i=0; i<10 && !found; ++i)
        if (p[(pos+i)%size]>0)
            found = true;

    if (found)
        return (float)(i%size)*(float)READBUFFERSIZE/(float)size;
    else
        return 0.f;
}

Reader *EngineBuffer::getReader()
{
    return reader;
}

float EngineBuffer::getBpm()
{
    return bpmControl->get();
}

void EngineBuffer::setOtherEngineBuffer(EngineBuffer *pOtherEngineBuffer)
{
    if (!m_pOtherEngineBuffer)
        m_pOtherEngineBuffer = pOtherEngineBuffer;
    else
        qFatal("EngineBuffer: Other engine buffer already set!");
}

void EngineBuffer::setNewPlaypos(double newpos)
{
    filepos_play = newpos;
    bufferpos_play = 0.;

    // Update bufferposSlider
    bufferposSlider->set((CSAMPLE)bufferpos_play);
    absPlaypos->set(filepos_play);

    // Ensures that the playpos slider gets updated in next process call
    m_iSamplesCalculated = 1000000;

    m_iBeatMarkSamplesLeft = 0;
}

const char *EngineBuffer::getGroup()
{
    return group;
}

float EngineBuffer::getRate()
{
    return rateSlider->get();
}

void EngineBuffer::setTemp(double v)
{
    m_dTemp = v;
}

void EngineBuffer::setTempSmall(double v)
{
    m_dTempSmall = v;
}

void EngineBuffer::setPerm(double v)
{
    m_dPerm = v;
}

void EngineBuffer::setPermSmall(double v)
{
    m_dPermSmall = v;
}



void EngineBuffer::slotControlSeek(double change)
{
    //qDebug("seeking... %f",change);

    // Find new playpos
    double new_playpos = round(change*file_length_old);
    if (!even((int)new_playpos))
        new_playpos--;
    if (new_playpos > file_length_old)
        new_playpos = file_length_old;
    if (new_playpos < 0.)
        new_playpos = 0.;

    // Seek reader
    //qDebug("seek %f",new_playpos);
    reader->requestSeek(new_playpos);

    m_iBeatMarkSamplesLeft = 0;
//    filepos_play_exchange.write(filepos_play);
//    file->seek((long unsigned)filepos_play);
//    visualPlaypos.tryWrite(
}

// Set the cue point at the current play position:
void EngineBuffer::slotControlCueSet(double)
{
    double cue = round(filepos_play);
    if (!even((int)cue))
        cue--;
    reader->f_dCuePoint = cue;
    cuePoint->set(cue);
}

// Goto the cue point:
void EngineBuffer::slotControlCueGoto(double pos)
{
    if (pos!=0.)
    {
        // Set cue point if play is not pressed
        if (playButton->get()==0.)
        {
            slotControlCueSet();

            // Start playing
            playButton->set(1.);
        }
        else
        {
            // Seek to cue point
            reader->requestSeek(reader->f_dCuePoint);
            m_iBeatMarkSamplesLeft = 0;
        }
    }
}

void EngineBuffer::slotControlCuePreview(double)
{
    // Set cue point if play is not pressed
    if (playButton->get()==0.)
        slotControlCueSet();

    if (buttonCuePreview->get()==0.)
    {
        // Stop playing (set playbutton to stoped) and seek to cue point
        playButton->set(0.);
        m_bCuePreview = false;
        reader->requestSeek(reader->f_dCuePoint);
        m_iBeatMarkSamplesLeft = 0;
    }
    else if (!m_bCuePreview)
    {
        // Seek to cue point and start playing
        m_bCuePreview = true;

        if (playButton->get()==0.)
            playButton->set(1.);
        else
        {
            // Seek to cue point
            reader->requestSeek(reader->f_dCuePoint);
            m_iBeatMarkSamplesLeft = 0;
        }
    }
}

void EngineBuffer::slotControlPlay(double)
{
    slotControlCueSet();
}

void EngineBuffer::slotControlStart(double)
{
    slotControlSeek(0.);
}

void EngineBuffer::slotControlEnd(double)
{
    slotControlSeek(1.);
}

void EngineBuffer::slotSetBpm(double bpm)
{
    ReaderExtractBeat *beat = reader->getBeatPtr();
    if (beat!=0)
    {
        // Get file BPM
        CSAMPLE *bpmBuffer = beat->getBpmPtr();
        double filebpm = bpmBuffer[(int)(bufferpos_play*(beat->getBufferSize()/READCHUNKSIZE))];

//        qDebug("user %f, file %f, change %f",bpm, filebpm, bpm/filebpm);

        // Change rate to match new bpm
        rateSlider->set(bpm/filebpm-1.);
    }
}

void EngineBuffer::slotControlRatePermDown(double)
{
    // Adjusts temp rate down if button pressed
    if (buttonRatePermDown->get()==1.)
        rateSlider->sub(m_dPerm);
}

void EngineBuffer::slotControlRatePermDownSmall(double)
{
    // Adjusts temp rate down if button pressed
    if (buttonRatePermDownSmall->get()==1.)
        rateSlider->sub(m_dPermSmall);
}

void EngineBuffer::slotControlRatePermUp(double)
{
    // Adjusts temp rate up if button pressed
    if (buttonRatePermUp->get()==1.)
        rateSlider->add(m_dPerm);
}

void EngineBuffer::slotControlRatePermUpSmall(double)
{
    // Adjusts temp rate up if button pressed
    if (buttonRatePermUpSmall->get()==1.)
        rateSlider->add(m_dPermSmall);
}

void EngineBuffer::slotControlRateTempDown(double)
{
    // Adjusts temp rate down if button pressed, otherwise set to 0.
    if (buttonRateTempDown->get()==1. && !m_bTempPress)
    {
        m_bTempPress = true;
        rateSlider->sub(m_dTemp);
    }
    else if (buttonRateTempDown->get()==0.)
    {
        m_bTempPress = false;
        rateSlider->add(m_dTemp);
    }
}

void EngineBuffer::slotControlRateTempDownSmall(double)
{
    // Adjusts temp rate down if button pressed, otherwise set to 0.
    if (buttonRateTempDownSmall->get()==1. && !m_bTempPress)
    {
        m_bTempPress = true;
        rateSlider->sub(m_dTempSmall);
    }
    else if (buttonRateTempDownSmall->get()==0.)
    {
        m_bTempPress = false;
        rateSlider->add(m_dTempSmall);
    }
}

void EngineBuffer::slotControlRateTempUp(double)
{
    // Adjusts temp rate up if button pressed, otherwise set to 0.
    if (buttonRateTempUp->get()==1. && !m_bTempPress)
    {
        m_bTempPress = true;
        rateSlider->add(m_dTemp);
    }
    else if (buttonRateTempUp->get()==0.)
    {
        m_bTempPress = false;
        rateSlider->sub(m_dTemp);
    }
}

void EngineBuffer::slotControlRateTempUpSmall(double)
{
    // Adjusts temp rate up if button pressed, otherwise set to 0.
    if (buttonRateTempUpSmall->get()==1. && !m_bTempPress)
    {
        m_bTempPress = true;
        rateSlider->add(m_dTempSmall);
    }
    else if (buttonRateTempUpSmall->get()==0.)
    {
        m_bTempPress = false;
        rateSlider->sub(m_dTempSmall);
    }
}

void EngineBuffer::slotControlBeatSync(double)
{
    float fOtherBpm = m_pOtherEngineBuffer->getBpm();
    float fThisBpm  = bpmControl->get();
    float fRateScale;

    if (fOtherBpm>0. && fThisBpm>0.)
    {
        // Test if this buffers bpm is the double of the other one, and find rate scale:
        if ( fabs(fThisBpm*2.-fOtherBpm) < fabs(fThisBpm-fOtherBpm))
            fRateScale = fOtherBpm/(2*fThisBpm) * (1.+m_pOtherEngineBuffer->getRate());
        else if ( fabs(fThisBpm-2.*fOtherBpm) < fabs(fThisBpm-fOtherBpm))
            fRateScale = 2.*fOtherBpm/fThisBpm * (1.+m_pOtherEngineBuffer->getRate());
        else
            fRateScale = fOtherBpm/fThisBpm * (1.+m_pOtherEngineBuffer->getRate());

        // Ensure the rate is within resonable boundaries
        if (fRateScale<2. || fRateScale>0.5)
            // Adjust the rate:
            rateSlider->set(fRateScale-1.);
    }


/*
    // Search for distance from playpos to beat mark of both buffers
    float fThisDistance = 2.; //getDistanceNextBeatMark();
    float fOtherDistance = 20.; //m_pOtherEngineBuffer->getDistanceNextBeatMark();

    qDebug("this %f, other %f",fThisDistance, fOtherDistance);

    filepos_play += fOtherDistance-fThisDistance;
    bufferpos_play = bufferpos_play+fOtherDistance-fThisDistance;
    if (bufferpos_play>(double)READBUFFERSIZE)
        bufferpos_play -= (double)READBUFFERSIZE;

    qDebug("buffer pos %f, file pos %f",bufferpos_play,filepos_play);
*/
}

CSAMPLE *EngineBuffer::process(const CSAMPLE *, const int buf_size)
{
    //Q_ASSERT( scale->getNewPlaypos() == 0);
    // pause can be locked if the reader is currently loading a new track.
    if (m_pTrackEnd->get()==0 && pause.tryLock())
    {
        // Try to fetch info from the reader
        bool readerinfo = false;
        long int filepos_start = 0;
        long int filepos_end = 0;
        if (reader->tryLock())
        {
            file_length_old = reader->getFileLength();
            file_srate_old = reader->getFileSrate();
            filepos_start = reader->getFileposStart();
            filepos_end = reader->getFileposEnd();
            reader->setFileposPlay((int)filepos_play);

            reader->unlock();
            readerinfo = true;
        }

        //qDebug("filepos_play %f,\tstart %i,\tend %i\t info %i, len %i",filepos_play, filepos_start, filepos_end, readerinfo,file_length_old);



        //
        // Calculate rate
        //

        // Find BPM adjustment factor
        ReaderExtractBeat *beat = reader->getBeatPtr();
        double bpmrate = 1.;
        double filebpm = 0.;
        if (beat!=0)
        {
            CSAMPLE *bpmBuffer = beat->getBpmPtr();
            filebpm = bpmBuffer[(int)(bufferpos_play*(beat->getBufferSize()/READCHUNKSIZE))];
        }

        // Determine direction of playback from reverse button
        double dir = 1.;
        if (reverseButton->get()==1.)
            dir = -1.;

        double baserate = dir*bpmrate*((double)file_srate_old/(double)getPlaySrate());
        if (fwdButton->get()==1.)
            baserate = fabs(baserate)*5.;
        else if (backButton->get()==1.)
            baserate = fabs(baserate)*-5.;

        double rate;
        if (playButton->get()==1. || fwdButton->get()==1. || backButton->get()==1.)
            rate=wheel->get()+(1.+rateSlider->get()*m_pRateDir->get())*baserate;
        else
            rate=wheel->get()*baserate*20.;

/*
        //
        // Beat event control. Assume forward play
        //

        // Search for next beat
        ReaderExtractBeat *readerbeat = reader->getBeatPtr();
        bool *beatBuffer = (bool *)readerbeat->getBasePtr();
        int nextBeatPos;
        int beatBufferPos = bufferpos_play*((CSAMPLE)readerbeat->getBufferSize()/(CSAMPLE)READBUFFERSIZE);
        int i;
        for (i=beatBufferPos+1; i<beatBufferPos+readerbeat->getBufferSize(); i++)
            if (beatBuffer[i%readerbeat->getBufferSize()])
                break;
        if (beatBuffer[i%readerbeat->getBufferSize()])
            // Next beat was found
            nextBeatPos = (i%readerbeat->getBufferSize())*(READBUFFERSIZE/readerbeat->getBufferSize());
        else
            // No next beat was found
            nextBeatPos = bufferpos_play+buf_size;

        double event = beatEventControl->get();
        if (event > 0.)
        {
            qDebug("event: %f, playpos %f, nextBeatPos %i",event,bufferpos_play,nextBeatPos);
            //
            // Play next event
            //

            // Reset beat event control
            beatEventControl->set(0.);

            if (oldEvent>0.)
            {
                // Adjust bufferplaypos
                bufferpos_play = nextBeatPos;

                // Search for a new next beat position
                ReaderExtractBeat *readerbeat = reader->getBeatPtr();
                bool *beatBuffer = (bool *)readerbeat->getBasePtr();

                int beatBufferPos = bufferpos_play*((CSAMPLE)readerbeat->getBufferSize()/(CSAMPLE)READBUFFERSIZE);
                int i;
                for (i=beatBufferPos+1; i<beatBufferPos+readerbeat->getBufferSize(); i++)
                {
    //                qDebug("i %i",i);
                    if (beatBuffer[i%readerbeat->getBufferSize()])
                        break;
                }
                if (beatBuffer[i%readerbeat->getBufferSize()])
                    // Next beat was found
                    nextBeatPos = (i%readerbeat->getBufferSize())*(READBUFFERSIZE/readerbeat->getBufferSize());
                else
                    // No next beat was found
                    nextBeatPos = -1;
            }

            oldEvent = 1.;
        }
        else if (oldEvent==0.)
            nextBeatPos = -1;

//        qDebug("NextBeatPos :%i, bufDiff: %i",nextBeatPos,READBUFFERSIZE/readerbeat->getBufferSize());
*/



        // If the rate has changed, write it to the rate_exchange monitor
        if (rate != rate_old)
        {
            // The rate returned by the scale object can be different from the wanted rate!
            rate_old = rate;
            rate = scale->setRate(rate);
            rate_old = rate;
            rate_exchange.tryWrite(rate);
        }

        bool at_start = false;
        bool at_end = false;

        // Determine playback direction
        bool backwards = false;
        if (rate<0.)
            backwards = true;

        //qDebug("rate: %f, playpos: %f",rate,playButton->get());

        if ((rate==0.) || (filepos_play==0. && backwards) || (filepos_play==(float)file_length_old && !backwards))
        {
            for (int i=0; i<buf_size; i++)
                buffer[i]=0.;
        }
        else
        {
            // Check if we are at the boundaries of the file
            if ((filepos_play<0. && backwards) || (filepos_play>file_length_old && !backwards))
            {
                qDebug("buffer out of range");
                for (int i=0; i<buf_size; i++)
                    buffer[i] = 0.;
            }
            else
            {
                // Perform scaling of Reader buffer into buffer
                CSAMPLE *output = scale->scale(bufferpos_play, buf_size);
                int i;
                for (i=0; i<buf_size; i++)
                    buffer[i] = output[i];
                double idx = scale->getNewPlaypos();

                // If a beat occours in current buffer mark it by led or in audio
                // This code currently only works in forward playback.
                ReaderExtractBeat *readerbeat = reader->getBeatPtr();
                if (readerbeat!=0)
                {
                    // Check if we need to set samples from a previos beat mark
                    if (audioBeatMark->get()==1. && m_iBeatMarkSamplesLeft>0)
                    {
                        int to = (int)min(m_iBeatMarkSamplesLeft, idx-bufferpos_play);
                        for (int j=0; j<to; j++)
                            buffer[j] = 30000.;
                        m_iBeatMarkSamplesLeft = max(0,m_iBeatMarkSamplesLeft-to);
                    }

                    float *beatBuffer = (float *)readerbeat->getBasePtr();
                    int chunkSizeDiff = READBUFFERSIZE/readerbeat->getBufferSize();
                    //qDebug("from %i-%i",(int)floor(bufferpos_play),(int)floor(idx));
                    //Q_ASSERT( (int)floor(bufferpos_play) <= (int)floor(idx) );
                    for (i=(int)floor(bufferpos_play); i<=(int)floor(idx); i++)
                    {
                        if (((i%chunkSizeDiff)==0) && (beatBuffer[i/chunkSizeDiff]>0.))
                        {
//                            qDebug("%i: %f",i/chunkSizeDiff,beatBuffer[i/chunkSizeDiff]);

                            // Audio beat mark
                            if (audioBeatMark->get()==1.)
                            {
                                int from = (int)(i-bufferpos_play);
                                int to = (int)min(i-bufferpos_play+audioBeatMarkLen, idx-bufferpos_play);
                                for (int j=from; j<to; j++)
                                    buffer[j] = 30000.;
                                m_iBeatMarkSamplesLeft = max(0,audioBeatMarkLen-(to-from));

                                qDebug("audio beat mark");
                                //qDebug("mark %i: %i-%i", i2/chunkSizeDiff, (int)max(0,i-bufferpos_play),(int)min(i-bufferpos_play+audioBeatMarkLen, idx));
                            }
#ifdef __UNIX__
                            // PowerMate led
                            if (powermate!=0)
                                powermate->led();
#endif
                        }
                    }
                }

                // Write file playpos
                filepos_play += (idx-bufferpos_play);
                if (file_length_old>0 && filepos_play>file_length_old)
                {
                    idx -= filepos_play-file_length_old;
                    filepos_play = file_length_old;
                    at_end = true;
                    //qDebug("end");
                }
                else if (filepos_play<0)
                {
                    //qDebug("start %f",filepos_play);
                    idx -= filepos_play;
                    filepos_play = 0;
                    at_start = true;
                }

                // Ensure valid range of idx
                while (idx>READBUFFERSIZE)
                    idx -= (double)READBUFFERSIZE;
                while (idx<0)
                    idx += (double)READBUFFERSIZE;

                // Write buffer playpos
                bufferpos_play = idx;

                //qDebug("bufferpos_play %f, old %f",idx,oldidx);
            }
        }

        //
        // Check if more samples are needed from reader, and wake it up if necessary.
        //
        if (readerinfo && filepos_end>0)
        {
            //qDebug("play %f,\tstart %i,\tend %i\t info %i, len %i, %f<%f",filepos_play, filepos_start, filepos_end, readerinfo,file_length_old,fabs(filepos_play-filepos_start),(float)(READCHUNKSIZE*(READCHUNK_NO/2-1)));
//            qDebug("checking");
            // Part of this if condition is commented out to ensure that more block is
            // loaded at the end of an file to fill the buffer with zeros
            if ((filepos_end - filepos_play < READCHUNKSIZE*(READCHUNK_NO/2-1)))
            {
                //qDebug("wake fwd play %f, end %i",filepos_play, filepos_end);
                reader->wake();
            }
            else if (fabs(filepos_play-filepos_start)<(float)(READCHUNKSIZE*(READCHUNK_NO/2-1)))
            {
                //qDebug("wake back");
                reader->wake();
            }
            else if (filepos_play>filepos_end || filepos_play<filepos_start)
            {
                reader->requestSeek(filepos_play);
                //reader->wake();
            }


            //
            // Check if end or start of file, and playmode, write new rate, playpos and do wakeall
            // if playmode is next file: set next in playlistcontrol
            //

            // Update playpos slider and bpm display if necessary
            m_iSamplesCalculated += buf_size;
            if (m_iSamplesCalculated > (44100/UPDATE_RATE))
            {
                if (file_length_old!=0.)
                {
                    double f = max(0.,min(filepos_play,file_length_old));
                    playposSlider->set(f/file_length_old);
                }
                else
                    playposSlider->set(0.);
                bpmControl->set(filebpm);

                m_iSamplesCalculated = 0;
            }

            // Update bufferposSlider
            bufferposSlider->set((CSAMPLE)bufferpos_play);
            absPlaypos->set(filepos_play);
        }

        //qDebug("filepos_play %f, len %i, back %i, play %f",filepos_play,file_length_old, backwards, playButton->get());
        // If playbutton is pressed, check if we are at start or end of track
        if (playButton->get()==1. && m_pTrackEnd->get()==0. &&
            ((filepos_play<=0. && backwards==true) ||
             ((int)filepos_play>=file_length_old && backwards==false)))
        {
            // If end of track mode is set to next, signal EndOfTrack to TrackList,
            // otherwise start looping, pingpong or stop the track
            int m = (int)m_pTrackEndMode->get();
            qDebug("end mode %i",m);
            switch (m)
            {
            case TRACK_END_MODE_STOP:
                qDebug("stop");
                playButton->set(0.);
                m_pTrackEnd->set(1.);
                break;
            case TRACK_END_MODE_NEXT:
                qDebug("next");
                m_pTrackEnd->set(1.);
                break;
            case TRACK_END_MODE_LOOP:
                qDebug("loop");
                slotControlSeek(0.);
                break;
/*
            case TRACK_END_MODE_PING:
                qDebug("Ping not implemented yet");

                if (reverseButton->get()==1.)
                    reverseButton->set(0.);
                else
                    reverseButton->set(1.);

                break;
*/
            default:
                qDebug("Invalid track end mode: %i",m);
            }
        }

        pause.unlock();

    }
    else
    {
        for (int i=0; i<buf_size; i++)
            buffer[i]=0.;
    }

    return buffer;
}

