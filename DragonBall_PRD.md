<!-- AI 上下文注入区：每次开启新对话时，请优先读取本区块 -->
## 🚨 [Live] 实时开发状态 (截至 2026-04-12)
> **核心记忆：当前处于 Phase 2 完成，即将进入 Phase 3。**

### 🛠️ 技术实现变更 (重要)
1. **SFML 3 适配**：
    - `FloatRect` 不再有 `width/height`，必须使用 `size.x / size.y`。
    - `sf::Sprite` 删除了无参构造函数，必须绑定 Texture 创建。当前使用 `std::unique_ptr<sf::Sprite>` 管理。
    - `setOrigin` 和 `setScale` 必须传入 `{x, y}` 格式的 `Vector2f`，不能传两个浮点数。
2. **资源路径**：
    - 实际路径：`assets/characters/Vegito/Vegito_Blue_Kaioken_0-1.png` (组号 - 编号.png)。
    - CMake 已配置自动复制 `assets` 文件夹到运行目录。
3. **代码结构**：
    - `ResourceManager` 使用了单例模式 + `unique_ptr<Texture>` 缓存。
    - `Fighter` 物理逻辑已跑通（重力、跳跃、地面吸附、左右镜像）。

### 🏁 当前进度
- **Phase 1**: ✅ 完成 (主循环、输入、窗口)
- **Phase 2**: ✅ 完成 (角色显示、物理移动、SFML 3 兼容性修复)
- **Phase 3**: 🔄 **待启动** (解析 .air 文件、多帧动画播放器)

---
# 🐉 龙珠 2D 格斗游戏 (DragonBall 2D Fighter) 技术需求与实施指南

> **版本**：v1.2 | **目标读者**：C++ 初学者 / 独立游戏开发者  
> **技术栈**：`C++20` + `SFML 3.0.2` + `CMake 3.16+`  
> **核心资产**：Mugen 格式素材 (`Vegito_Blue_Kaioken` PNG序列 + `.def/.air/.cns` 配置文件)

---

## 🏗️ 1. 核心架构设计模式

本项目采用 **数据驱动 + 状态机驱动 + 组件化** 的轻量级架构：

| 模块 | 设计模式 | 技术实现要点 |
|:---|:---|:---|
| **输入系统** | 快照模式 (Snapshot) | 每帧轮询按键生成 `InputState` 结构体，状态机只读快照，避免帧内状态撕裂 |
| **资源管理** | 享元模式 (Flyweight) + RAII | `std::unordered_map<std::string, std::unique_ptr<sf::Texture>>` 缓存纹理，避免重复 I/O |
| **角色逻辑** | 状态模式 (State Pattern) | `StateMachine` 持有 `std::unique_ptr<FighterState>`，通过虚函数 `enter/update/exit` 驱动行为 |
| **对象创建** | 抽象工厂 (Factory) | `FighterFactory` 根据配置文件动态实例化角色，支持热插拔新人物 |
| **碰撞检测** | 空间划分 (AABB) | 轴对齐包围盒检测，帧内只计算 `HitBox` 与 `HurtBox` 的矩形交集 |
| **动画播放** | 状态机 + 定时器 | `AnimationPlayer` 维护 `currentFrame` 与 `frameTimer`，基于 `dt` 累加切换 |

---

## 🚀 2. 分阶段实施指南 (含技术细节)

> 📌 **重要原则**：严格按阶段创建文件，**不要提前创建后续阶段的空文件**，否则会导致 C++ 链接器报错 (`undefined reference`)。

---

### 🔨 Phase 1: 骨架搭建与输入控制
**🎯 目标**：创建 SFML 3 窗口，实现主循环，读取键盘输入并输出状态快照。

#### 📂 需要新建的文件/文件夹
```plain text
DragonBall/
├── CMakeLists.txt
├── main.cpp
├── include/
│   └── Core/
│       ├── Game.h
│       └── InputManager.h
└── src/
    └── Core/
        ├── Game.cpp
        └── InputManager.cpp
```


#### ⚙️ 技术实现细节
| 模块 | 关键数据结构 / API | 实现逻辑 |
|:---|:---|:---|
| **主循环** | `sf::Clock`, `sf::RenderWindow`, `sf::Event` | 使用 `dt = clock.restart().asSeconds()` 计算帧时间。循环结构：`处理事件 → 更新逻辑 → 清屏 → 绘制 → display()` |
| **事件处理 (SFML 3)** | `window.pollEvent()` 返回 `std::optional<sf::Event>` | 使用 C++17/20 `if (event->is<sf::Event::Closed>())` 替代旧版 `event.type == sf::Event::Closed` |
| **输入快照** | `struct InputState { bool up, down, left, right, punch; }` | 每帧调用 `sf::Keyboard::isKeyPressed()` 填充结构体。禁止在状态机内直接轮询键盘，实现**逻辑与输入解耦** |
| **CMake 配置** | `find_package(SFML ...)`, `target_link_libraries` | 必须链接 `SFML::Window` 和 `SFML::Graphics`。添加 `POST_BUILD` 命令自动复制 `SFML/bin/*.dll` 到输出目录 |

**🏁 验收标准**：
- 编译通过，窗口标题显示 `DragonBall 2D`
- 控制台实时打印 `Input: [W:0][A:1][J:0]...`
- 点击关闭或按 `ESC` 安全退出，无内存泄漏

---

### 🏃 Phase 2: 角色渲染与基础物理
**🎯 目标**：加载贝吉特待机图，实现重力、跳跃、左右移动与画面镜像。

#### 📂 需要新增的文件/文件夹
```plain text
├── include/
│   └── Characters/
│       ├── Fighter.h
│       ├── AnimationPlayer.h
│       └── ResourceManager.h
├── src/
│   └── Characters/
│       ├── Fighter.cpp
│       ├── AnimationPlayer.cpp
│       └── ResourceManager.cpp
├── assets/
│   └── characters/Vegito/0-1.png  (待机帧)
```


#### ⚙️ 技术实现细节
| 模块 | 关键数据结构 / API | 实现逻辑 |
|:---|:---|:---|
| **资源管理器** | `std::unordered_map<std::string, std::unique_ptr<sf::Texture>>` | 单例模式。`loadTexture(path)` 检查缓存，不存在则 `new sf::Texture()` 并 `loadFromFile()`。返回 `sf::Sprite` 时绑定缓存纹理指针 |
| **物理模拟** | `sf::Vector2f position, velocity` | 欧拉积分：`velocity.y += gravity * dt; position += velocity * dt;`。落地检测：`if (position.y >= GROUND_Y) { position.y = GROUND_Y; velocity.y = 0; }` |
| **绘制与翻转** | `sf::Sprite::setScale()`, `setOrigin()` | 向左移动时 `sprite.setScale(-1.f, 1.f)`。需重新计算 `origin` 为 `(width/2, height/2)` 防止翻转后坐标偏移 |
| **动画播放器 (V1)** | `sf::Sprite`, `sf::Texture*` | 暂不处理序列。仅暴露 `setTexture()`, `setPosition()`, `draw(window)` 接口 |

**🏁 验收标准**：
- 窗口中央显示贝吉特图片
- `← →` 控制平滑移动，`↑` 起跳后受重力自然落下
- 面向左右自动镜像，图片不闪烁、不越界

---

### 🎞️ Phase 3: 动画系统与 Mugen 数据解析
**🎯 目标**：解析 `.air` 文件，实现多帧序列动画播放，支持偏移量校正。

#### 📂 需要新增的文件/文件夹
```plain text
├── include/
│   └── Utils/
│       ├── MugenParser.h
│       └── ConfigLoader.h
├── src/
│   └── Utils/
│       ├── MugenParser.cpp
│       └── ConfigLoader.cpp
├── config/
│   └── game.ini
├── assets/
│   └── Vegito_Blue_Kaioken_ULSW.air  (动画定义文件)
```


#### ⚙️ 技术实现细节
| 模块 | 关键数据结构 / API | 实现逻辑 |
|:---|:---|:---|
| **文本解析器** | `std::ifstream`, `std::string_view`, `std::stoi` | 逐行读取。跳过注释 (`;`)。匹配 `[Begin Action X]` 提取组号。解析 `G,N,X,Y,T` 格式：使用 `std::find(',')` 分割，转整数 |
| **动画数据模型** | `struct AnimFrame { std::string path; int duration; sf::Vector2i offset; };`<br>`struct Animation { int id; std::vector<AnimFrame> frames; bool loop; };` | 将解析结果存入 `std::map<int, Animation>`。路径拼接逻辑：`fmt::format("assets/characters/Vegito/{}_{}-{}.png", prefix, group, num)` |
| **播放器升级** | `float frameTimer`, `size_t currentFrame` | `update(dt)` 中累加时间，`>= duration` 时切换帧并重置计时器。支持 `loop=false` 停在最后一帧或触发回调 |
| **C++20 特性应用** | `std::format` (或 `fmt` 库), `std::span` | 路径生成、日志输出使用 `std::format`。避免 C 风格字符串拼接，提升安全性与可读性 |

**🏁 验收标准**：
- 解析 `.air` 成功，控制台打印动画 ID 与帧数
- 走路/待机时自动播放对应序列帧，无跳帧、卡顿
- 支持从文件读取的 `offset` 修正脚底贴合地面

---

### 🎭 Phase 4: 状态机 (FSM) 与行为逻辑
**🎯 目标**：实现有限状态机，规范动作切换，解决输入冲突与逻辑死锁。

#### 📂 需要新增的文件/文件夹
```plain text
├── include/
│   └── States/
│       ├── StateMachine.h
│       ├── FighterState.h      (基类)
│       └── FighterStates.h     (派生类声明)
├── src/
│   └── States/
│       ├── StateMachine.cpp
│       └── FighterStates.cpp   (Idle, Walk, Jump, Attack 实现)
```


#### ⚙️ 技术实现细节
| 模块 | 关键数据结构 / API | 实现逻辑 |
|:---|:---|:---|
| **状态基类** | `virtual void enter(Fighter&)=0;`<br>`virtual void update(Fighter&, const InputState&, float dt)=0;`<br>`virtual void exit(Fighter&)=0;` | 遵循 **开闭原则**。新增状态只需派生新类，不修改状态机核心代码 |
| **状态机管理** | `std::unique_ptr<FighterState> currentState;` | `changeState(std::unique_ptr<FighterState> next)`：调用旧状态 `exit()` → 替换指针 → 调用新状态 `enter()`。使用 `std::move` 转移所有权 |
| **优先级仲裁** | `enum class Priority { IDLE=0, MOVE=1, ATTACK=2, HURT=3 };` | `FighterState::getPriority()`。状态切换前检查：`if (next->getPriority() >= current->getPriority()) allow` |
| **输入消费** | `InputState::consume(jump)` | 攻击状态锁定期间，忽略移动输入。受击状态清空输入缓冲。防止“空中走路”、“攻击中闪避”等 Bug |

**🏁 验收标准**：
- 按 `J` 进入攻击状态，期间移动/跳跃无效
- 攻击动画结束后自动回归 `Idle`
- 空中无法触发 `Walk`，落地瞬间平滑过渡
- 状态切换时控制台打印 `[State] Idle -> Jump`

---

### ⚔️ Phase 5: 战斗系统与 UI
**🎯 目标**：实现 AABB 碰撞框、伤害计算、击退反馈、HUD 血量条。

#### 📂 需要新增的文件/文件夹
```plain text
├── include/
│   ├── Combat/
│   │   ├── Hitbox.h
│   │   └── DamageSystem.h
│   └── UI/
│       └── HUD.h
├── src/
│   ├── Combat/
│   │   ├── Hitbox.cpp
│   │   └── DamageSystem.cpp
│   └── UI/
│       └── HUD.cpp
├── resources/
│   └── fonts/arial.ttf  (或思源黑体)
```


#### ⚙️ 技术实现细节
| 模块 | 关键数据结构 / API | 实现逻辑 |
|:---|:---|:---|
| **碰撞框模型** | `struct HitBox { sf::FloatRect rect; int damage; bool active; sf::Vector2f knockback; };`<br>`struct HurtBox { sf::FloatRect rect; };` | 每帧根据动画当前帧更新框体坐标。攻击动画第 N 帧激活 `HitBox.active = true` |
| **AABB 检测** | `rect1.left < rect2.left + rect2.width && rect1.left + rect1.width > rect2.left && ...` | 纯数学计算，无浮点误差容忍。命中后标记 `HitBox.isConsumed = true` 防止单帧多次扣血 |
| **伤害结算** | `target.health -= hit.damage; target.velocity += hit.knockback;` | 触发后切换目标至 `HitState`。加入受击停顿 (`hitStop`)：主循环 `sleep(0.016)` 或跳过渲染帧实现顿帧感 |
| **HUD 渲染** | `sf::RectangleShape` (血条), `sf::Text` (字体) | 血条宽度 `ratio * MAX_WIDTH`。颜色插值：`sf::Color::lerp(green, red, 1-ratio)`。使用 `sf::Font::loadFromFile` 预加载中文字体 |

**🏁 验收标准**：
- 双角色同屏，P1 攻击命中 P2 触发击退与扣血
- P2 血量归零播放倒地动画，显示 `K.O.` 文字
- 血条颜色随血量动态变化，UI 不遮挡角色主体

---

## 🛠️ 3. 关键技术实现规范

### 3.1 资源生命周期管理 (RAII)
```c++
// ✅ 正确：使用智能指针管理纹理，避免悬垂引用
class ResourceManager {
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures;
public:
    sf::Sprite getSprite(const std::string& path) {
        if (!textures.contains(path)) {
            textures[path] = std::make_unique<sf::Texture>();
            textures[path]->loadFromFile(path);
        }
        sf::Sprite s(*textures[path]); // 绑定缓存
        return s;
    }
};
```


### 3.2 SFML 3 事件循环标准写法
```c++
// SFML 3 使用 std::optional 和类型安全检查
while (const auto event = window.pollEvent()) {
    if (event->is<sf::Event::Closed>()) {
        window.close();
    }
    if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) window.close();
    }
}
```


### 3.3 物理步进与渲染解耦
```c++
// 固定时间步长物理更新 (60Hz)
float accumulator = 0.0f;
const float fixedDt = 1.0f / 60.0f;

while (running) {
    float dt = clock.restart().asSeconds();
    accumulator += dt;
    while (accumulator >= fixedDt) {
        handleInput();
        updatePhysics(fixedDt); // 物理/逻辑固定步长
        checkCollisions();
        accumulator -= fixedDt;
    }
    render(dt); // 渲染使用实际帧时间平滑插值
}
```


---

## ⚙️ 4. CMake 构建与调试规范

### 4.1 现代化 CMake 模板
```cmake
cmake_minimum_required(VERSION 3.16)
project(DragonBall LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SFML 3 查找 (注意组件大写)
find_package(SFML 3.0.2 COMPONENTS Window Graphics Audio CONFIG REQUIRED)

# 自动收集源码 (推荐新手手动添加，此处展示自动方案)
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(DragonBall main.cpp ${SOURCES})
target_include_directories(DragonBall PRIVATE include)

# 链接库
target_link_libraries(DragonBall PRIVATE SFML::Window SFML::Graphics SFML::Audio)

# 编译后自动复制 DLL 到运行目录 (Windows)
if(WIN32)
    add_custom_command(TARGET DragonBall POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "D:/SFML-3.0.2/bin"
        $<TARGET_FILE_DIR:DragonBall>
    )
endif()
```


### 4.2 调试宏与日志系统
```c++
#ifdef DB_DEBUG
    #define LOG_INFO(fmt, ...) std::cout << "[INFO] " << std::format(fmt, __VA_ARGS__) << "\n"
    #define DRAW_HITBOXES
#else
    #define LOG_INFO(...)
#endif
```

- **断点调试**：在 `StateMachine::changeState` 和 `Fighter::applyDamage` 处打断点
- **性能监控**：使用 `sf::Clock` 统计 `update()` 与 `render()` 耗时，确保 `< 16.6ms`

---

## 📅 5. 开发路线图与验收清单

| 阶段 | 核心任务 | 预计耗时 | 交付物 | 风险点 |
|:---|:---|:---|:---|:---|
| **Phase 1** | 窗口/主循环/输入快照 | 2-3 天 | 可交互黑窗 | SFML 3 语法不熟 |
| **Phase 2** | 纹理加载/重力物理/镜像 | 3-4 天 | 可跳跃的静态图 | 纹理生命周期管理 |
| **Phase 3** | .air 解析/序列动画/偏移 | 4-5 天 | 流畅动画播放 | 字符串解析边界处理 |
| **Phase 4** | 状态机/优先级/输入消费 | 5-6 天 | 无冲突动作切换 | 状态泄露/野指针 |
| **Phase 5** | 碰撞框/伤害/UI/结算 | 6-8 天 | 完整对战 demo | AABB 穿透/帧同步 |
