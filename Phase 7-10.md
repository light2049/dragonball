这是一份专为**新窗口无缝交接**设计的完整项目实现文档（PRD）。请直接复制以下 Markdown 内容并保存为 `.md` 文件（或直接粘贴到新聊天窗口），新 AI 读取后即可获得完整的项目全景图和后续开发指令。

***

# 🐉 DragonBall 2D Fighter Engine - 完整项目交接文档 (v1.0)

## 1. 📚 项目基础概况
- **项目名称**: DragonBall 2D Fighter
- **技术栈**: C++20, SFML 3.0.2, CMake, MinGW64
- **核心架构**: **数据驱动 + 混合逻辑**
    - **C++ 核心层**: 负责确定性逻辑（物理/重力计算、输入缓冲、渲染管线、坐标空间变换）。
    - **数据驱动层**: 解析 MUGEN 格式的 `.cns` (状态逻辑) 和 `.air` (动画/判定数据)，决定角色行为。
- **开发原则**:
    - **拒绝硬编码**: 所有战斗逻辑（攻击判定、伤害数值、状态流转）尽量由数据文件驱动。
    - **模块化**: `Fighter` (角色实体), `StateRegistry` (状态管理), `AirParser/CnsParser` (数据解析)。
    - **代码规范**: 严格使用 SFML 3 语法，使用 RAII，避免裸指针。

---

## 2. ✅ Phase 6 完成情况总结 (Current State)
**阶段目标**: 实现数据驱动的状态机与 CNS 集成，完成角色基础运动闭环。

### 🚀 已实现功能
1.  **CNS 解析与注册**:
    *   实现了 `CnsParser::loadStateDefs()`，成功从 `.cns` 提取 `[Statedef XXX]`。
    *   `StateRegistry` 建立了状态索引表，支持 `hasState()` 和 `executeState()` 接口。
2.  **核心状态机 (State Machine)**:
    *   角色能够基于输入在以下状态间丝滑切换：
        *   **0 (Idle/Stand)**: 待机
        *   **20 (Walk)**: 行走
        *   **40 (Jump Start) / 50 (Jump Up)**: 起跳与空中
        *   **200 (Attack)**: 攻击
3.  **动画同步 (Animation Sync)**:
    *   状态切换时，`AnimationPlayer` 根据当前朝向和状态 ID 自动播放对应动画（如 20→Walk, 200→Attack）。
    *   解决了旧版动画闪烁和黑屏问题。
4.  **物理基础 (Physics)**:
    *   实现了重力 (`GRAVITY`)、速度积累和地面碰撞检测 (`GROUND_Y = 480.f`)。
    *   解决了 Fighter 和 Dummy 高度不一致的问题。
5.  **架构清理**:
    *   移除了早期硬编码的碰撞扣血逻辑。
    *   修复了 `CnsParser` 中 `StateDef` 结构体的链接错误，统一了类型定义。

### 📂 当前关键文件结构
```plain text
include/
 ├── Characters/
 │    ├── Fighter.h       (核心角色逻辑，状态流转，物理控制)
 │    ├── AnimationPlayer.h
 │    └── Dummy.h
 ├── Core/
 │    ├── StateRegistry.h (管理 Statedef 解析结果，执行状态逻辑)
 │    ├── Game.h          (主循环，资源初始化)
 │    └── InputManager.h
 └── Utils/
      ├── AirParser.h     (解析 .air 文件，管理动画帧)
      ├── CnsParser.h     (解析 .cns 文件，包含 StateDef 结构体)
      └── CNSController.h (状态控制器基类)

src/
 ├── Characters/ (对应 cpp)
 ├── Core/       (对应 cpp)
 └── Utils/      (对应 cpp)
```


---

## 3. 🗺️ Phase 7 - Phase 10 路线图 (Next Steps)

### 🔷 Phase 7: 精确碰撞框解析与可视化 (Data-Driven Clsn)
**目标**: 彻底废除硬编码判定框，解析 `.air` 中的 Clsn 数据并实现可视化调试。
**关键任务**:
1.  **AirParser 升级**: 解析 `.air` 中的 `Clsn1` (受击框) 和 `Clsn2` (攻击框) 数据（格式通常为 `ClsnX: N` 后接 `X1, Y1, X2, Y2` 顶点）。
2.  **数据结构**: 在 `Frame` 中存储 `Clsn` 矩形列表。
3.  **坐标变换**: 实现 `Fighter::GetWorldHitbox()`，将素材的本地坐标系转换为世界坐标系。
    *   *算法*: `WorldPos = CharacterPos + (LocalClSn * Facing * Scale)`。
4.  **Debug 绘图**: 在屏幕上实时绘制半透明红色矩形 (Hurtbox) 和绿色矩形 (Hitbox)。
    **验收标准**: 判定框紧密贴合角色素材，且随动作、缩放、朝向实时精准变化。

### 🔶 Phase 8: HitDef 解析与战斗结算系统 (Combat Logic)
**目标**: 实现真正的格斗游戏战斗循环（伤害、硬直、HitStop）。
**关键任务**:
1.  **CNS HitDef 解析**: 解析 `type = HitDef` 中的关键参数：
    *   `damage` (基础伤害/防御伤害)
    *   `p2stateno` (受击者跳转的目标状态 ID)
    *   `hitflag` (判定生效的状态过滤)
    *   `pausetime` (命中停顿帧数)
2.  **碰撞检测激活**: 在状态执行期间，当 `Animation` 播放到包含 `Clsn2` 的帧时，激活攻击判定。
3.  **HitStop 系统**: 命中时冻结双方逻辑更新指定帧数，但保持画面渲染。
4.  **伤害结算**: 命中后扣除 HP，触发受击方跳转状态 (`ChangeState p2stateno`)。
    **验收标准**: 攻击有效判定触发扣血，屏幕出现命中停顿，受击者按设定状态受击。

### 🟣 Phase 9: 推撞系统 (Pushbox) 与防御机制 (Defense)
**目标**: 增强物理交互的真实感，实现防御系统。
**关键任务**:
1.  **Pushbox 防穿模**: 定义角色身体碰撞体积。每帧检测双方 Pushbox 重叠，若重叠则强制将两者沿 X 轴推开，防止模型互相穿插。
2.  **防御判定**:
    *   逻辑：检测输入方向为“背向对手”且处于站立/蹲下状态。
    *   状态：若满足条件，进入防御状态 (Guard State)。
3.  **防御结算**:
    *   拦截普通攻击判定，应用 `guard.damage` 逻辑（削减伤害）。
    *   触发防御硬直动画，无法移动。
        **验收标准**: 角色碰撞时自动推开不重叠；正确格挡后扣血减少并播放防御动画。

### 🟡 Phase 10: Round 流程与 UI 完善 (Game Loop)
**目标**: 完善 UI 表现，构建完整的单局对战流程。
**关键任务**:
1.  **HUD 完善**:
    *   实现血条动态平滑扣血动画（不仅仅是数字变化）。
    *   添加能量槽（Power Bar）显示。
2.  **Round 状态机**:
    *   管理流程：`Pre-Intro` (入场) -> `Round Start` (3, 2, 1, Fight) -> `Fighting` -> `K.O.` -> `Result` -> `Reset`。
3.  **胜负判定**: 生命值归零处理，胜利/失败动画播放，等待按键重置。
    **验收标准**: 跑通完整的“开局→对战→KO→结算→重开”流程，无状态残留或崩溃。

---

## 4. ⚠️ AI 开发注意事项
1.  **SFML 3 兼容性**: 所有的输入处理 (`sf::Event::KeyPressed` -> `sf::Event::KeyPressed::code`) 和向量初始化 (`sf::Vector2f{x, y}`) 必须兼容 SFML 3。
2.  **内存管理**: 避免内存泄漏，使用 `std::unique_ptr` 管理动态对象。
3.  **状态上下文**: 在修改 `Fighter` 逻辑时，务必考虑到 `requestStateChange` 可能发生的任何时间点（如空中、攻击中）。
4.  **浮点精度**: 涉及物理计算和坐标转换时，使用 `float` 或 `double`，避免整数截断导致的抖动。
