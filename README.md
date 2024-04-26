# Project Documentation

![Generic badge](https://img.shields.io/badge/version-1.0.0-green.svg)
![Generic badge](https://img.shields.io/badge/license-MIT-blue.svg)

## Table of Contents
1. [Overview](#overview)
2. [Files and Descriptions](#files-and-descriptions)
3. [Setup and Configuration](#setup-and-configuration)
4. [Related Repositories](#related-repositories)
5. [License](#license)

## Overview
This project is developed as part of a Master's thesis focused on demonstrating real-time GPS tracking using the LilyGO T-Impulse device. The primary function of this software is to enable the device to connect to The Things Network (TTN) and reliably report its geographic location. 


### Files and Descriptions

#### `Bat.cpp`
- **Purpose**: Manages battery related functionalities.
- **Key Functions**:
  - `bat_init()`: Initializes battery settings.
  - `bat_sleep()`: Enters low power mode.
  - `bat_loop()`: Monitors and updates battery status.

#### `energy_mgmt.cpp`
- **Purpose**: Manages power for components.
- **Key Functions**:
  - `BoardInit()`: Initializes components like GPS and OLED.
  - `Board_Sleep()`: Manages power-down sequences.

#### `gps.cpp`
- **Purpose**: Handles GPS functionality.
- **Key Functions**:
  - `gps_init()`: Initializes GPS reception.
  - `gps_sleep()`: Reduces power consumption.
  - `gps_loop()`: Maintains GPS data processing.

#### `loramac.cpp`
- **Purpose**: Manages LoRaWAN communication.
- **Key Functions**:
  - `setupLMIC()`: Configures LoRaWAN MAC.
  - `loopLMIC()`: Continues communication handling.
  - `do_send()`: Sends data packets via LoRaWAN.

#### `main.cpp`
- **Purpose**: Main entry for the firmware.
- **Key Functions**:
  - `setup()`: Prepares system setup.
  - `loop()`: Main execution loop.

#### `oled.cpp`
- **Purpose**: Controls OLED display.
- **Key Functions**:
  - `oled_init()`: Sets up the OLED display.
  - `oled_sleep()`: Enters display sleep mode.

#### `touch.cpp`
- **Purpose**: Detects touch inputs.
- **Key Functions**:
  - `TouchCallback()`: Measures touch duration.

## Setup and Configuration

This section provides a quick guide to setting up and configuring the project for the LilyGO T-Impulse device using PlatformIO, ensuring the device can connect to The Things Network (TTN) and report its location via GPS.

### Prerequisites

1. **Install PlatformIO**: Ensure that [PlatformIO](https://platformio.org/install) is installed in your development environment. PlatformIO is an open source ecosystem for IoT development and is required to manage project configurations, libraries, and board definitions.

2. **Clone the Repository**: Clone the project repository to your local machine using Git:
   ```
   git clone <repository-url>
   cd <project-directory>
   ```
   
3. **Secrets File Setup**:
    - Create a .secrets folder in the root of the project directory.
    - Inside the .secrets folder, create a file named secrets.h.
    - Define the LoRaWAN credentials in secrets.h as follows.
      ```
      #define APPEUI_SECRET "<your-app-eui>"
      #define DEVEUI_SECRET "<your-dev-eui>"
      #define APPKEY_SECRET "<your-app-key>"
      ```

### PlatformIO Configuration
The platformio.ini file contains all necessary configuration settings to compile and upload the firmware to the LilyGO T-Impulse device.

### Programming mode
- Connect the device USB to the PC.
- Hold down the B button.
- Press and release the R button.
- Release the B button.

### Compiling and Uploading
To compile and upload the firmware to the device, open the PlatformIO menu (when using VScode), Project Tasks, select the corresponding environment and click Upload.

## Related Repositories

This project builds upon the work found in several open-source repositories. Below is a list of these projects along with brief descriptions of how they contribute to the current project:

### [LilyGo-LoRa-Series](https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series)
**Description**: Repository containing LILYGO LoRa Series examples.

### [T-Impulse](https://github.com/Xinyuan-LilyGO/T-Impulse)
**Description**: Repository of LILYGO T-IMPULSE documentation and guides.

Each of these repositories has contributed to the foundation or the enhancement of the functionality in this project. For detailed information on how each component is used and integrated, refer to the specific module documentation.


## License

This project, developed as part of a Master's thesis, incorporates and builds upon various open-source software components. Each component retains its original license as specified by its creators. The modifications and new code in this project are released under the MIT License.

### MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.