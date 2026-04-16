# FormerGlove

## 🧠 Abstract

Effective strength training relies heavily on correct technique. However, many athletes perform exercises with poor form, which increases the risk of injury and reduces training efficiency. While professional coaching can solve this issue, it is often too expensive or inaccessible for the average gym-goer.

To address this problem, we developed **Former Glove** — a smart wearable system that acts as a digital coach.

The project consists of sensor-equipped gloves that track hand movements during weightlifting. Using intelligent software, the system analyzes exercises in real time to:

- Detect mistakes in form  
- Automatically count repetitions  

This feedback is transmitted wirelessly to a mobile application, allowing users to evaluate their performance immediately after each set.

By providing instant, automated guidance, Former Glove delivers the benefits of professional coaching without the high cost — helping users train more safely and effectively.

---

## 📁 Repository Structure

This repository contains the part of the project developed by **Daniel Jones**.

In addition to the main folders, various files document the development process of Former Glove.

### 📦 Folder Overview

- **`3D/`**  
  Contains all final versions of the 3D housing models, created using Autodesk Fusion.

- **`csv/`**  
  Includes movement datasets that were manually recorded for AI training.

- **`LeftGlove/` & `RightGlove/`**  
  Final code for each glove.  
  ⚠️ These projects must be opened separately, as the two Edge Impulse libraries will conflict during compilation.

- **`src/`**  
  Contains code used for testing and data collection.

---

## 🚀 Project Goal

Former Glove aims to make professional-level training feedback accessible to everyone by combining:

- Wearable hardware  
- Real-time motion tracking  
- Intelligent analysis software  

---

## 📌 Notes

- Make sure to open glove projects individually to avoid library conflicts  
- Data in `/csv` was used for training machine learning models  
- Development documentation is included in the root directory  

---

## 👤 Author

**Daniel Jones**