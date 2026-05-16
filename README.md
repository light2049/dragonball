# DragonBall 2D Fighting Game

基于 C++20 和 SFML 3.0.2 开发的 2D 格斗游戏，兼容 M.U.G.E.N 角色数据格式。

## 项目概述

本项目是一个从零实现的格斗游戏引擎，核心功能包括：

- 解析 M.U.G.E.N 角色文件（.air / .cns / .cmd / .sff）
- 状态机驱动的格斗系统（站立、行走、攻击、受击、倒地等）
- 碰撞检测（Clsn1 攻击框 vs Clsn2 受击框）
- 连段系统（多段命中、p1stateno / p2stateno 追击）
- 受击反馈（hitstop 命中停顿、hitflash 闪白、EnvShake 屏幕震动）
- 超必杀（SuperPause 特写暂停、superdarken 背景暗化）
- 特效系统（Explod、Helper、AfterImage 残影）
- 双人同屏对战（P1: WASD + JKLUIO，P2: 方向键 + 小键盘）
- 单人对战
- 普通攻击支持**长按按键触发连段（连续按键暂时还不能触发连段）**，不同角色连段数和衔接时机独立配置
- M.U.G.E.N 兼容的状态编号体系（-1 指令检测、-2/-3 通用逻辑）

## 环境要求

- C++20 编译器
- CMake 3.20+
- SFML 3.0.2

## 构建

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## 运行

直接运行 `build/DragonBall.exe`。

> **注意**：程序的工作目录为项目根目录，需要确保 `Data/` 文件夹存在于运行目录下。如果从 build 目录运行，需要将 `Data/` 复制到 build 目录

## 操作说明

### P1 键位

| 按键 | 功能 |
|------|------|
| W / A / S / D | 上 / 左 / 下 / 右 |
| J | 轻拳（x） |
| I | 中拳（y） |
| O | 重拳 / 超必杀（z） |
| K | 轻脚（a） |
| L | 中脚（b） |
| U | 重脚（c） |
| 空格 | 蓄气（s） |
| Enter | 确认 |

### P2 键位

| 按键 | 功能 |
|------|------|
| ↑ / ← / ↓ / → | 上 / 左 / 下 / 右 |
| 小键盘1 | 轻拳（x） |
| 小键盘5 | 中拳（y） |
| 小键盘6 | 重拳 / 超必杀（z） |
| 小键盘2 | 轻脚（a） |
| 小键盘3 | 中脚（b） |
| 小键盘4 | 重脚（c） |
| 小键盘0 | 确认（s） |

### 调试键

| 按键 | 功能 |
|------|------|
| F1 | 显示/隐藏调试信息 |
| F3 | 动画调试日志 |
| F11 | 切换全屏 |
| Esc | 退出全屏 / 关闭游戏 |

## 必杀技指令表

以自在极意功悟空为例，必杀技按下述方向组合加攻击键触发：

| 指令 | 说明 |
|------|------|
| ↓↘→ + J | 下前 + 轻拳 |
| ↓↙← + J | 下后 + 轻拳 |
| ↓↘→ + I | 下前 + 中拳 |
| ↓↙← + I | 下后 + 中拳 |
| ↓↘→ + K | 下前 + 轻脚 |
| ↓↙← + K | 下后 + 轻脚 |
| ↓↘→ + L | 下前 + 中脚 |
| ↓↙← + L | 下后 + 中脚 |
| ↓↘→ + U | 下前 + 重脚 |
| ↓↙← + U | 下后 + 重脚 |

↘ 表示同时按下 + 右，↙ 表示同时按下 + 左。方向输入后在限定时间内按下攻击键即可释放。气力消耗根据招式不同分为 500 / 1000 / 2000 / 3000 四档。不同角色的招式组合不完全相同。

## 角色列表

目前包含 9 个角色，均来自 M.U.G.E.N 社区（mugenarchive.com）：

| 角色      | 目录 | 精灵数 |
|---------|------|-----|
| 野兽悟饭    | Beast_Gohan | 544 |
| 常态悟吉塔   | Gogeta_BASE | 737 |
| 超蓝悟吉塔   | Gogeta_SSB | 671 |
| 超赛4悟吉塔  | Gogeta_SSJ4 | 534 |
| 自在极意功悟空 | Goku_MUI | 477 |
| 超赛3悟空   | Goku_SSJ3 | 477 |
| 超赛贝吉塔   | Vegeta_SSJ | 331 |
| 超赛贝吉特   | Vegetto_SSJ | 618 |
| 超蓝贝吉特   | Vegito_Blue_Kaioken | 443 |

添加新角色只需在 `Data/Characters/` 下新建文件夹，放入对应的 .def / .air / .cns / .cmd / .sff 等文件，无需修改代码。

## 项目结构

```
DragonBall/
├── CMakeLists.txt          构建配置
├── main.cpp                程序入口
├── include/                头文件
│   ├── Characters/         Fighter、AnimationPlayer
│   ├── Core/               Game、InputManager、StateRegistry
│   ├── UI/                 HUD、BitmapFont
│   └── Utils/              AirParser、CnsParser、CmdParser、SFFDatabase
├── src/                    源文件（与头文件对应）
└── Data/
    ├── Characters/          角色文件
    ├── Stages/              场景背景
    └── UI/                  界面贴图
```

## 核心架构

### 动画系统（AirParser + AnimationPlayer）

- `AirParser` 解析 .air 文件，返回 `map<int, Animation>`，编号对应动作
- `Animation` 包含 `vector<AnimFrame>`，每帧有贴图路径、轴心、持续时间和判定框
- `AnimationPlayer` 逐帧推进，支持循环、翻转和三种颜色混合模式

### 状态机（CnsParser + StateRegistry）

- 每个招式对应一个状态（StateDef），包含一组控制器（CNSController）
- 控制器类型：HitDef（攻击判定）、VelSet（速度）、SuperPause（暂停）、Explod（特效）等
- 每帧执行状态 -1（指令检测）→ 当前状态 → 状态 -2/-3（通用逻辑）
- 通过 `requestStateChange()` 切换状态，自动重置相关标志

### 碰撞检测

- `checkCombat()` 每帧检测双方攻击框与受击框的矩形重叠
- 命中后根据 `p2stateno` 或 `animtype` 自动选择受击状态
- 支持 p1stateno 连段追击

## 角色素材来源

所有角色文件均从 **mugenarchive.com** 下载，遵循 M.U.G.E.N 标准格式。素材版权归原作者所有，本项目仅作为引擎技术演示使用。

## License

本项目代码部分采用 MIT License。角色素材版权归各自原作者所有。
