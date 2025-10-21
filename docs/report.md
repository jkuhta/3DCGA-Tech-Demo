# 3DCGA - Tech Demo (Group 17)

**Authors:** Darian Samsoedien, Jan Kuhta  
**Course:** CS4515 – 3D Computer Graphics and Animation  
**Title:** 3D Tech Demo  
**Date:** 05.11.2025

---

## Group Members and Work Division

| Student Name      | Implemented Features  | Percentage Indication per Feature |
|-------------------|-----------------------|-----------------------------------|
| Darian Samsoedien | - Example Feature 1   | 5%                                |
|                   | - Example Feature 2   | 5%                                |
| Jan Kuhta         | - Multiple viewpoints | 5%                                |
|                   | - Example Feature 2   | 5%                                |

---

## Project Overview

This tech demo showcases multiple real-time rendering techniques implemented in C++ and OpenGL.  
All features are toggleable from the in-app UI, and the scene demonstrates the integration of advanced shading,
animation, and procedural effects.

## Implemented Features

Below is an overview of all implemented features and where they can be toggled in the UI.

| ID | Feature             | Description                                   | ImgUi               | Hotkey | Time spent |
|----|---------------------|-----------------------------------------------|---------------------|--------|------------|
| F1 | Multiple viewpoints | Switching between world view and object view. | Dropdown: Viewpoint | 1/2    | 3h         |

### F1. Multiple viewpoints

This feature allows the user to switch between the world view and the object view. World view is where the user can
move the camera freely around the scene (W, S, A, D, R, F and rotate with the mouse). Object view is where the
camera is fixed behind the object, and the user can move the object around (W, S, A, D) while the camera follows.

Dropdown: Viewpoint + select viewpoint option
Hotkey: 1 (World view) / 2 (Object view)

This feature was implemented similarly to exercise 4.1, utilizing the Camera class from camera.cpp. The World View
camera behaves similarly to exercise 4.1 (uses the same view matrix calculation). The Object View camera extends this
functionality by calling setFollowTarget(), which configures the camera to track a target position with a fixed offset.
The view matrix is then calculated to maintain this offset relationship. When the object moves, the camera always
follows the object from behind with the same offset. Switching between cameras works via the UI dropdown, and the
setUserInteraction() is called accordingly to choose the active camera. Active camera is then used to calculate the
view matrix which is passed to the shaders.

TODO: add screenshots

---

## Demo Video

**Video link:** [Add Google Drive link]
> The demo video provides an overview of all implemented techniques and UI toggles.  
> Duration: ≤ 5 minutes.  
> Commentary explains each feature as it is shown.

---

## References

Include citations or URLs for:

- Any external tutorials, documentation, or books used [1]
- Third-party models, textures, or assets (with licenses)
- Any code snippets or ideas adapted (clearly referenced)

[1] **UFO 3D Model**  
Nieves, H. (2013). UFO 3D model. Available at: https://free3d.com/3d-model/ufo-21509.html  
License: Free for personal and commercial use
