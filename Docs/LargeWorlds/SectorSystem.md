In order to represent accurate positions within a galaxy-scale environment, a "sector"-based system is used, where each sector is a volume in space. 
The sectors are indexed using a 3D vector of 64-bit signed integers, with [0,0,0] being the global center, and [0,0,-1] being the sector just "below" the central sector. 
To render geometry, positions are calculated in relation to the sector the camera currently occupies. 
Each physical object's position within the global environment is represented with a) sector coordinates and b) a floating-point vector. 
The floating-point vectors are relative to the center of the object's "home sector", instead of the traditional approach which uses a global origin (as seen in most other applications). 
This system maintains very good mathematical accuracy within any sector, and it can represent a large scale global environment up to a radius of INT64_MAX * SECTOR_WIDTH + (SECTOR_WIDTH/2).
At 20km, the smallest possible distance (epsilon) is roughly 0.00119m, providing millimeter-level precision throughout the entire environment. 
This is significantly better as compared to most video games, which often see degraded physics and animations at far distances, and thus are limited in size.

Requirements for the sector system: 
1. Each sector must be small enough to maintain good floating-point accuracy.
2. Each sector must be small enough for sector-local physics to work reliably within the entire sector.
3. The sectors must be large enough for the total effective volume to cover an entire galaxy.

The upper limit of a signed 64-bit integer is ~9.22e18. 
An appropriate sector width must be chosen to fit a galactic radius within this number while adhering to the precision requirements. 
To get the effective supported radius, it can be calculated as follows. For example if sectors are 20km wide: 

sector_width_meters = 20000
int64_max = (2**63) - 1

total_radius_meters = (int64_max * sector_width_meters) + (sector_width_meters // 2)

total_radius_lightyears = total_radius_meters / 9460730472580800
total_galactic_radii = total_radius_lightyears / 50000

We get an effective radius of over 184 sextillion meters, which corresponds to 19498223.87 lightyears.
Nearly 390 times larger than the radius of our real galaxy, assuming the Milky Way is aproximately 50 000 lightyears in radius.  
At 20km, it is not enough to represent the entire observable universe, but there is room for multiple galaxies.
 
In conclusion, even at a sector width of just 1km, this system provides enough space for - at the very least - an entire galaxy to be represented at true scale.
