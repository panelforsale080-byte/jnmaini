#pragma once


enum EAim {
    Distance = 0,
    Crosshair = 1,
};

enum EAimTarget {
    Heads = 0,
    Chests = 1,
    Body = 2,
    AimTargetBot = 3,
};


enum PlayerStatus {
    NORMAL = 1,
    DEAD = 2,
    OFFLINE = 3,
    KNOCKDOWN = 4
};


enum EAimTrigger {
    Shooting = 1,
    Scoping = 2,
    Hybrid = 3,
};
