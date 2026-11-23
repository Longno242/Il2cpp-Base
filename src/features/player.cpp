#include "features/player.hpp"

#include "features/players.hpp"

namespace il2cpp_base::features::player {

void update() {
    void* local = players::local_player();
    il2cpp::Class* klass = players::player_class();
    health.apply(local, klass);
    stamina.apply(local, klass);
    ammo.apply(local, klass);
    money.apply(local, klass);
}

} // namespace il2cpp_base::features::player
