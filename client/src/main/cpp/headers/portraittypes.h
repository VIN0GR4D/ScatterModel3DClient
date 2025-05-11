#ifndef PORTRAITTYPES_H
#define PORTRAITTYPES_H

enum PortraitDimension {
    Undefined = -1,
    Range = 0,       // Дальностный
    Azimuth = 1,     // Азимутальный
    Elevation = 2,   // Угломестный
    AzimuthRange = 3,    // Комбинация Азимутальный+Дальностный
    ElevationRange = 4,  // Комбинация Угломестный+Дальностный
    ElevationAzimuth = 5, // Комбинация Угломестный+Азимутальный
    ThreeDimensional = 6  // Комбинация Угломестный+Азимутальный+Дальностный
};

#endif // PORTRAITTYPES_H