# Ostrostroj

## Installation
1. Build binary locally using CMake
1. Copy binary to the device (user's home directory): `scp .\build\src\ostrostroj ostrostroj:`
1. Copy startup script to the device: `scp .\scripts\ostrostroj.service ostrostroj:`
1. Connect to the device `ssh ostrostroj`
    1. Move binary: `sudo mv ~/ostrostroj /usr/local/bin/`
    1. Move startup script: `sudo mv ~/ostrostroj.service /etc/systemd/system/`
    1. Install: `sudo systemd install ostrostroj.service`
    1. Start: `sudo systemctl start ostrostroj`
    1. Enable auto start on boot: `sudo systemctl enable ostrostroj`
    1. Create workspace `sudo mkdir /srv/ostrostroj`
        1. Read, write for everyone: `sudo chmod -R 0775 /srv/ostrostroj`

## Running
- Option 1 manual run: `ostrostroj /srv/ostrostroj/`
- Option 2 run as a service :
    - `sudo service ostrostroj start`
    - `sudo service ostrostroj stop`

## Logs

- `journalctl -u ostrostroj`
- `service ostrostroj status`

## Development

- For faster deployment cycle:
    - `sudo chown rado:rado /usr/local/bin/ostrostroj`
    - `scp .\build\src\ostrostroj ostrostroj:/usr/local/bin/ostrostroj`

## Users Guide

This guide assumes that Elektron Syntakt is used as master MIDI device and Elektron Model:Cycles is used as a slave device (MIDI connections: Syntakt -> Ostrostroj -> M:C).

### Initial Setup

#### 1. Workspace Structure
Create following directory structure and copy (`scp`) audio files to it:
- Workspace root directory (`/srv/ostrostroj`)
    - Project directory (`01_domovy_poriadok`)
        - Song directory (`A01_kratke_dni`)
            - Section directory (`01_verse`)
                - Mono loop audio files (`L1.wav` - `L4.wav`)
                - Stereo loop audio files (`L5.wav`, `L6.wav`)
            - Another section directory (`02_chorus`)
                - Loop files...
                - *Hint: use symlinks to reuse existing loop files*
            - One shot files (`S1.wav`, `S2_.wav`, `S5_melody.wav`, ...)
        - Another song directory (`A05_menej`)

#### 2. Initialize New Project in Elektron Syntakt
1. Create new project
1. `Settings -> MIDI Config -> Sync`: enable Clock, Transport and Program Change send.

#### 3. Initialize MIDI Track 8 For a New Pattern
1. Switch to a (new) pattern (`A01` by default)
1. Switch to Track 8
    1. Machine: MIDI
    1. SYN PAGE (MIDI Source): Channel 8
    1. FLT PAGE (CC Value): enable all by setting them to 0 (zero) by pressing FUNC + encoder
    1. AMP PAGE (CC Select):
        1. Loops (mute + saturation):
            1. CC1 Select=CC #111
            1. CC2 Select=CC #112
            1. CC3 Select=CC #113
            1. CC4 Select=CC #114
            1. CC5 Select=CC #115
            1. CC6 Select=CC #116
        1. One-shot:
            1. CC8 Select=CC #119

#### 4. Initialize Keyboard for Track 8
1. KB SCALE = CHRO
1. ROOT NOTE = C4
1. KB FOLD = ON

### Display

![](doc/UnicornHATMini_2of3_1500x1500_crop_center.jpg "Pimoroni Mini Hat")

### Song Mode

### Live Controls