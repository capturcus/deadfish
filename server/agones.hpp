#pragma once

namespace agones {

// returns true if starting succeeded and false otherwise
bool Start();
void Shutdown();
void SetPlayers(int players);
void SetPlaying();

}; // agones
