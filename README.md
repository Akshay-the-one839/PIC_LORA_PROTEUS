
# PIC_LORA_PROTEUS

This repository contains PIC CCS code projects and Proteus simulation files for LoRa and RTOS experiments.

## 📂 Folder Structure

PIC_LORA_PROTEUS/
├── PIC/
│   ├── Lora_Rtos/          # PIC CCS code with FreeRTOS
│   └── PIC_Lora/           # PIC CCS code for LoRa communication
└── PROTEUS/                # Proteus simulation files



## ⚡ Requirements

- Compiler → **CCS PIC C Compiler**  
- Simulation → **Proteus** (v8 or later recommended)  
- Target MCU → **PIC18F25K22** (or your chosen model)

## 🚀 Quick Start – How to Use

1. Open either  
   `PIC/Lora_Rtos`   or   `PIC/PIC_Lora`  
   folder in **CCS IDE**

2. Compile → get your `.hex` file(s)

3. In **Proteus**:  
   → Open file from `PROTEUS` folder  
   → Load `.pdsprj` project

4. Double-click the PIC in schematic  
   → Browse & select your freshly compiled `.hex`

5. Hit **Play** ▶ and watch it run!

## 🔧 Important Notes

- PIC model must be **exactly the same** in CCS and Proteus  
- LoRa modules → same **frequency + settings** on both sides  
- `Lora_Rtos` folder shows **FreeRTOS multitasking** in action

---

