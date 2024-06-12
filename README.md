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
        1. Read, write, execute for everyne: `sudo chmod -R 0777 /srv/ostrostroj`

## Workspace structure
- Workspace root directory (`/srv/ostrostroj`)
    - Project directory (`01_domovy_poriadok`)
        - Song directory (`A01_kratke_dni`)
            - Section directory (`01_verse`)
                - Mono loop audio files (`L1.wav` - `L4.wav`)
                - Stereo loop audio files (`L5.wav`, `L6.wav`)
            - Another section directory (`02_chorus`)
                - Loop files...
            - One shot files (`S0.wav`, `S1_.wav`, `S3_melody.wav`, ...)
        - Another song directory (`A05_menej`)

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