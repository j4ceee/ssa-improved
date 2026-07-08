#pragma once

namespace ssa::Game::FreeCam
{
    // toggle free cam on / off
    void Toggle();

    void Update();

    [[nodiscard]] bool IsActive();
}
