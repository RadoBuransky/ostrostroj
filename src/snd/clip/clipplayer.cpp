#include "common.hpp"
#include "clipplayer.hpp"

void ClipPlayer::update_pointers(ClipBlock& _block) {
    current_frame = _block.get_buffer().data();
    end_frame = current_frame + (_block.get_buffer_frames() * _block.get_channels());
}

ClipPlayer::ClipPlayer(Clip& _clip, bool _loop):
    clip(_clip),
    loop(_loop),
    block(_clip.get_head()),
    current_frame(nullptr),
    end_frame(nullptr),
    position(0),
    draining(false) {
    update_pointers(block.get());
}

void ClipPlayer::drain() {
    draining = true;
}