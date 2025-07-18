// 大黑山的诱惑
// 包含图形库、Windows API、多媒体等头文件
#include <windows.h>
#include <graphics.h>    // EasyX图形库，用于绘图
#include <mmsystem.h>    // 多媒体接口，用于播放音乐
#include <vector>        // 动态数组，存储矿石对象
#include <string>
#include <cmath>         // 数学函数，用于计算距离、角度
#include <ctime>         // 时间函数，用于随机数种子
#include <memory> 
#pragma comment(lib, "MSIMG32.LIB")  // 链接透明绘制所需的库
#pragma comment(lib, "Winmm.LIB")    // 链接多媒体库
using namespace std;

// 窗口常量定义
const int WIN_WIDTH = 1080;    // 窗口宽度
const int WIN_HEIGHT = 640;    // 窗口高度
const int GAME_FRAME = 60;     // 游戏帧率（每秒60帧）
const int MAX_ANGLE = 80;      // 钩子最大摆动角度（度）
class GameState {
public:
    // 游戏流程状态
    bool isInMenu = true;
    bool gameOver = false;
    bool showReturnBtn = false;
    bool isInInstruction = false;
    bool isPlayingCutScene = false;
    bool isRunning = true;

    // 背景音乐状态
    bool isMenuBGMPlaying = false;
    bool isInstructionBGMPlaying = false;
    bool isLaunchBGMPlaying = false;
    bool isPullBGMPlaying = false;
    bool isTimeupBGMPlaying = false;

    // 菜单与成就相关
    bool achieve_first = false;
    bool achieve_screw = false;
    bool achieve_bomb = false;
    bool achieve_underdev = false;
    bool achieve_perfectdev = false;
    bool achieve_overdev = false;
    bool gotNewAchievement = false;
    int lastAchieveCount = 0;

    // 游戏数据
    int tempHighScore = 0;
    int totalScore = 0;
    int gameTime = 40;
    int envDamage = 0;
    int currentLevel = 1;
    const int MAX_LEVEL = 5;
    int levelTimes[5] = { 40, 50, 60, 50, 40 };
    int baseScoreReduction = 0;

    // 单例模式实现
    static GameState& Instance() {
        static GameState instance;
        return instance;
    }

private:
    GameState() = default;  // 私有构造函数，确保单例
    GameState(const GameState&) = delete;
    GameState& operator=(const GameState&) = delete;
};

// 按钮位置（矩形区域）
RECT returnBtn{ 440, 400, 640, 460 }; // 返回按钮位置（居中）

// 图片资源（全局声明，后续加载）
IMAGE menuBG, mineBG, instructionBG, cutSceneImg, achieveBG, qiuxiaoshiImg, marryNewImg;
IMAGE hookImg, catchImg, goldImg, rockImg, diamondImg, screwImg, bombImg, coalImg, ironImg;
IMAGE ending1, ending2, ending3, achieve_first_img, achieve_screw_img, achieve_bomb_img,
achieve_underdev_img, achieve_perfectdev_img, achieve_overdev_img, lockedImg;

/**
 * 透明绘制图片
 * @param x 绘制位置x坐标
 * @param y 绘制位置y坐标
 * @param img 要绘制的图片
 * 功能：使用AlphaBlend实现图片的透明效果，避免背景遮挡
 */
void putimage_alpha(int x, int y, IMAGE* img) {
    int w = img->getwidth();   // 获取图片宽度
    int h = img->getheight();  // 获取图片高度
    HDC dstDC = GetImageHDC(NULL);  // 目标设备上下文（窗口）
    HDC srcDC = GetImageHDC(img);   // 源设备上下文（图片）
    // 透明混合参数：使用源图片的Alpha通道
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    // 执行透明绘制
    AlphaBlend(dstDC, x, y, w, h, srcDC, 0, 0, w, h, bf);
}

/**
 * 显示关卡过场动画
 * @param level 关卡号
 * 功能：在关卡开始前显示动画，提示当前关卡，持续3秒
 */
void ShowCutScene(int level) {
    GameState& state = GameState::Instance();
    state.isPlayingCutScene = true;               // 标记正在播放过场
    ULONGLONG startTime = GetTickCount64(); // 记录开始时间
    const int CUTSCENE_DURATION = 3000;     // 持续时间3000毫秒（3秒）

    // 播放过场音乐
    mciSendString(_T("play cutscene from 0"), NULL, 0, NULL);

    // 循环显示过场动画，直到时间结束
    while (GetTickCount64() - startTime < CUTSCENE_DURATION) {
        ExMessage msg;
        while (peekmessage(&msg)) {};  // 清空消息队列，避免交互

        // 绘制过场背景图（拉伸适配窗口）
        StretchBlt(GetImageHDC(NULL), 0, 0, WIN_WIDTH, WIN_HEIGHT,
            GetImageHDC(&cutSceneImg), 0, 0,
            cutSceneImg.getwidth(), cutSceneImg.getheight(), SRCCOPY);

        // 显示关卡文字（如“第一关”）
        setbkmode(TRANSPARENT);         // 文字背景透明
        settextstyle(60, 0, _T("微软雅黑")); // 文字样式
        settextcolor(WHITE);            // 文字颜色白色
        TCHAR levelText[32];
        _stprintf_s(levelText, _T("第%d关"), level);  // 格式化关卡文字
        SIZE sz;
        GetTextExtentPoint32(GetImageHDC(), levelText, _tcslen(levelText), &sz); // 获取文字尺寸
        // 居中显示关卡文字
        outtextxy((WIN_WIDTH - sz.cx) / 2, WIN_HEIGHT / 2 - 30, levelText);

        FlushBatchDraw();  // 刷新绘制
        Sleep(16);         // 控制帧率（约60帧）
    }

    // 结束过场动画
    mciSendString(_T("stop cutscene"), NULL, 0, NULL);  // 停止音乐
    state.isPlayingCutScene = false;  // 标记过场结束
}

/**
 * 成就菜单类
 * 功能：管理成就的显示、解锁状态及返回菜单交互
 */
class AchieveMenu {
public:
    IMAGE* bg;                  // 成就界面背景图
    RECT backBtn{ 830, 50, 1020, 110 };  // 返回按钮位置
    GameState& state;  // 引用游戏状态
    // 构造函数：初始化背景图
    AchieveMenu(IMAGE* bgImg) : bg(bgImg), state(GameState::Instance()) {}

    // 成就信息结构体：存储单个成就的状态和显示内容
    struct AchieveInfo {
        bool unlocked;          // 是否解锁
        IMAGE* unlockedImg;     // 解锁后的图片
        const TCHAR* title;     // 成就标题
        const TCHAR* desc;      // 成就描述
    };

    /**
     * 绘制单个成就
     * @param x 绘制x坐标
     * @param y 绘制y坐标
     * @param info 成就信息
     * 功能：根据解锁状态绘制成就（解锁显示图片和文字，未解锁显示锁图标）
     */
    void DrawSingleAchieve(int x, int y, const AchieveInfo& info) {
        // 选择显示的图片（解锁/未解锁）
        IMAGE* img = info.unlocked ? info.unlockedImg : &lockedImg;
        // 选择显示的文字（解锁/未解锁）
        const TCHAR* title = info.unlocked ? info.title : _T("???");
        const TCHAR* desc = info.unlocked ? info.desc : _T("???");

        // 绘制成就图片
        putimage_alpha(x, y, img);

        // 绘制成就标题和描述
        setbkmode(TRANSPARENT);
        settextcolor(BLACK);  // 文字颜色黑色

        // 绘制标题
        settextstyle(30, 0, _T("微软雅黑"));
        outtextxy(x + 90, y, title);  // 标题在图片右侧

        // 绘制描述（自动换行）
        settextstyle(24, 0, _T("微软雅黑"));
        RECT descRect = { x + 90, y + 35, x + 1200, y + 100 };  // 描述区域
        drawtext(desc, &descRect, DT_WORDBREAK);  // 自动换行绘制
    }

    /**
     * 运行成就菜单
     * 功能：循环显示成就界面，处理返回按钮点击
     */
    void Run() {
        ExMessage msg;  // 消息变量

        // 定义所有成就的信息（顺序与成就变量对应）
        AchieveInfo achieves[] = {
            {state.achieve_first, &achieve_first_img, _T("首次开采"), _T("为了泥开!")},
            {state.achieve_screw, &achieve_screw_img, _T("食堂螺丝"), _T("传说这是六食堂冰柜掉落的螺丝,难道是被风吹上大黑山的?(bushi")},
            {state.achieve_bomb, &achieve_bomb_img, _T("这是？炸药？"), _T("据说当年泥开二期开发使用了大量炸药,结果被周边邻居投诉了...所以你最好也不要用,能不用就别用吧!")},
            {state.achieve_underdev, &achieve_underdev_img, _T("开采不足"), _T("你该怎么向院长和泥开交代?")},
            {state.achieve_perfectdev, &achieve_perfectdev_img, _T("完美开发"), _T("重振泥开荣光,我辈义不容辞!")},
            {state.achieve_overdev, &achieve_overdev_img, _T("开发过度"), _T("不要小看泥开跟大黑山的羁绊啊!")},
        };

        const int lineSpacing = 90;  // 成就之间的行距
        const int startX = 100, startY = 80;  // 首个成就的起始位置

        // 成就界面主循环
        while (true) {
            // 绘制背景图
            StretchBlt(GetImageHDC(NULL), 0, 0, WIN_WIDTH, WIN_HEIGHT,
                GetImageHDC(bg), 0, 0, bg->getwidth(), bg->getheight(), SRCCOPY);

            // 绘制每个成就
            for (int i = 0; i < 6; i++) {
                int y = startY + i * lineSpacing;  // 计算第i个成就的y坐标
                DrawSingleAchieve(startX, y, achieves[i]);
            }

            // 绘制返回按钮
            setfillcolor(RGB(100, 200, 100));  // 按钮颜色（浅绿色）
            fillrectangle(backBtn.left, backBtn.top, backBtn.right, backBtn.bottom);  // 绘制按钮矩形
            settextcolor(BLACK);
            settextstyle(40, 0, _T("微软雅黑"));
            outtextxy(backBtn.left + 20, backBtn.top + 10, _T("  返回菜单"));  // 按钮文字

            FlushBatchDraw();  // 刷新绘制

            // 处理点击事件
            while (peekmessage(&msg)) {
                if (msg.message == WM_LBUTTONDOWN) {  // 左键点击
                    // 检查是否点击返回按钮
                    if (PtInRect(&backBtn, { msg.x, msg.y })) return;  // 退出成就界面
                }
            }
        }
    }
};
AchieveMenu achieveMenu(&achieveBG);  // 实例化成就菜单

/**
 * 主菜单类
 * 功能：管理主菜单界面（开始游戏、帮助、成就按钮）及界面切换
 */
class Menu {
public:
    IMAGE* bg;  // 菜单背景图
    GameState& state;
    // 按钮位置（开始游戏、帮助、成就）
    RECT btn_start{ 390,200,690,270 }, btn_help{ 390,300,690,370 }, btn_achieve{ 390,400,690,470 };

    // 构造函数：初始化背景图
    Menu(IMAGE* bgImg) : bg(bgImg) , state(GameState::Instance() ){}

    /**
     * 运行菜单
     * 功能：循环显示菜单，处理按钮点击，返回操作结果（1=开始游戏，0=其他）
     */
    int Run() {
        ExMessage msg;  // 消息变量
        while (true) {
            settextstyle(32, 0, _T("微软雅黑"), 0, 0, 700, 0, 0, 0);

            // 如果在游戏说明界面
            if (state.isInInstruction) {
                // 播放说明界面背景音乐（仅首次进入时）
                if (!state.isInstructionBGMPlaying) {
                    mciSendString(_T("stop menu"), NULL, 0, NULL);  // 停止菜单音乐
                    mciSendString(_T("play instruction repeat from 0"), NULL, 0, NULL);  // 播放说明音乐
                    state.isInstructionBGMPlaying = true;
                    state.isMenuBGMPlaying = false;
                }

                // 绘制说明界面背景
                StretchBlt(GetImageHDC(NULL), 0, 0, WIN_WIDTH, WIN_HEIGHT,
                    GetImageHDC(&instructionBG), 0, 0,
                    instructionBG.getwidth(), instructionBG.getheight(), SRCCOPY);

                // 绘制玩法说明文字
                setbkmode(TRANSPARENT);
                settextcolor(YELLOW);
                settextstyle(32, 0, _T("微软雅黑"), 0, 0, 700, 0, 0, 0);
                RECT textRect = { 100, 50, WIN_WIDTH - 100, WIN_HEIGHT - 100 };  // 文字区域
                const TCHAR* text = _T("玩法说明：左键发射钩子,勾取到矿石时可以右键放下,勾取到炸药时可以按空格引爆\n剧情简介: 连理大学开发区校区一直都在拖连理大学分数线的后腿(bushi\n于是乎,为吸引下一届学子,泥开大刀阔斧地扩建ing...\n为了助力泥开的施工,marry new无法抵御住大黑山的诱惑,决定派遣求小实去开发大黑山的矿产资源\n他能够完成任务吗?\n友情提醒: 要挖够5000分,但挖太多求小实会产生挖矿成本,消耗分数,且越深层消耗越多哦。注意环境,不要忘了泥开和大黑山二十多年的羁绊啊!\n免责声明: 本剧情纯属虚构,如有雷同,纯属巧合");
                drawtext(text, &textRect, DT_WORDBREAK);  // 自动换行

                // 绘制返回菜单按钮
                setfillcolor(RGB(100, 200, 100));  // 按钮颜色
                fillrectangle(returnBtn.left, returnBtn.top, returnBtn.right, returnBtn.bottom);
                settextstyle(40, 0, _T("微软雅黑"));
                settextcolor(BLACK);
                outtextxy(returnBtn.left + 20, returnBtn.top + 10, _T("  返回菜单"));

                // 绘制左下角角色图片（求小实）
                int leftBottomX = 150;
                int leftBottomY = WIN_HEIGHT - qiuxiaoshiImg.getheight();
                putimage_alpha(leftBottomX, leftBottomY, &qiuxiaoshiImg);

                // 绘制右下角角色图片（marry new）
                int rightBottomX = WIN_WIDTH - marryNewImg.getwidth();
                int rightBottomY = WIN_HEIGHT - marryNewImg.getheight();
                putimage_alpha(rightBottomX, rightBottomY, &marryNewImg);

                FlushBatchDraw();  // 刷新绘制

                // 处理说明界面的点击事件
                while (peekmessage(&msg)) {
                    if (msg.message == WM_LBUTTONDOWN) {  // 左键点击
                        int x = msg.x, y = msg.y;
                        // 点击返回按钮，回到主菜单
                        if (PtInRect(&returnBtn, { x, y })) {
                            mciSendString(_T("stop instruction"), NULL, 0, NULL);  // 停止说明音乐
                            state.isInInstruction = false;  // 标记退出说明界面
                            return 0;  // 返回菜单主循环
                        }
                    }
                }
            }
            // 主菜单界面
            else {
                // 播放菜单背景音乐（仅首次进入时）
                if (!state.isMenuBGMPlaying) {
                    mciSendString(_T("play menu repeat from 0"), NULL, 0, NULL);  // 播放菜单音乐
                    state.isMenuBGMPlaying = true;
                    state.isInstructionBGMPlaying = false;
                }

                // 绘制菜单背景
                StretchBlt(GetImageHDC(NULL), 0, 0, WIN_WIDTH, WIN_HEIGHT, GetImageHDC(bg), 0, 0, bg->getwidth(), bg->getheight(), SRCCOPY);

                // 显示当前最高分
                setbkmode(TRANSPARENT);
                settextstyle(50, 0, _T("微软雅黑"));
                settextcolor(BLACK);
                TCHAR highScoreText[64];
                _stprintf_s(highScoreText, _T("当前最高分：%d"), state.tempHighScore);
                outtextxy(20, 20, highScoreText);  // 左上角显示

                // 显示新成就提示
                if (state.gotNewAchievement) {
                    setbkmode(TRANSPARENT);
                    settextstyle(50, 0, _T("微软雅黑"));
                    settextcolor(RED);
                    outtextxy(WIN_WIDTH - 300, 20, _T("你获得了新成就！"));
                }

                // 显示游戏标题
                setbkmode(TRANSPARENT);
                settextstyle(108, 0, _T("微软雅黑"));
                settextcolor(RGB(255, 105, 180));  // 粉色标题
                SIZE sz;
                GetTextExtentPoint32(GetImageHDC(), _T("大黑山的诱惑"), _tcslen(_T("大黑山的诱惑")), &sz);
                outtextxy((WIN_WIDTH - sz.cx) - 375, 60, _T(" 大 黑 山 的 诱 惑 "));  // 居中显示

                // 绘制三个按钮（开始游戏、帮助、成就）
                settextstyle(40, 0, _T("微软雅黑"));
                setfillcolor(RGB(100, 200, 100));  // 按钮颜色（浅绿色）
                fillrectangle(btn_start.left, btn_start.top, btn_start.right, btn_start.bottom);  // 开始游戏按钮
                fillrectangle(btn_help.left, btn_help.top, btn_help.right, btn_help.bottom);      // 帮助按钮
                fillrectangle(btn_achieve.left, btn_achieve.top, btn_achieve.right, btn_achieve.bottom);  // 成就按钮

                // 按钮文字
                settextcolor(BLACK);
                outtextxy(btn_start.left + 30, btn_start.top + 10, _T("       开始游戏"));
                outtextxy(btn_help.left + 30, btn_help.top + 10, _T("       游戏说明"));
                outtextxy(btn_achieve.left + 30, btn_achieve.top + 10, _T("       成就系统"));

                FlushBatchDraw();  // 刷新绘制

                // 处理主菜单点击事件
                while (peekmessage(&msg)) {
                    if (msg.message == WM_LBUTTONDOWN) {  // 左键点击
                        int x = msg.x, y = msg.y;
                        if (PtInRect(&btn_start, { x,y })) return 1;  // 点击开始游戏，返回1
                        if (PtInRect(&btn_help, { x,y })) {  // 点击帮助
                            mciSendString(_T("stop menu"), NULL, 0, NULL);  // 停止菜单音乐
                            state.isInInstruction = true;  // 标记进入说明界面
                            return 0;
                        }
                        if (PtInRect(&btn_achieve, { x,y })) {  // 点击成就
                            state.gotNewAchievement = 0;  // 进入成就界面后，新成就提示消失
                            achieveMenu.Run();  // 运行成就菜单
                            return 0;
                        }
                    }
                }
            }
        }
    }
};

/**
 * 爆炸动画类
 * 功能：管理爆炸效果的帧动画（加载帧图片、播放动画）
 */
class ExplodeAnimation {
public:
    POINT pos;          // 爆炸位置
    int frame = 0;      // 当前帧
    bool active = false;// 动画是否激活
    static const int maxFrame = 24;  // 总帧数
    static unique_ptr<IMAGE> frames[maxFrame];

    /**
     * 加载爆炸动画资源
     * 功能：从文件加载所有爆炸帧图片
     */
    static void LoadResources() {
        for (int i = 0; i < maxFrame; i++) {
            TCHAR path[64];
            _stprintf_s(path, _T("image\\explode%d.png"), i);  // 图片路径（explode0.png到explode23.png）
            frames[i] = std::make_unique<IMAGE>();  // 自动管理内存
            loadimage(frames[i].get(), path);
        }
    }

    /**
     * 开始爆炸动画
     * @param p 爆炸中心位置
     * 功能：初始化动画状态，准备播放
     */
    void Start(POINT p) {
        pos = p;
        frame = 0;
        active = true;
    }

    /**
     * 绘制爆炸动画
     * 功能：根据当前帧绘制爆炸图片，自动推进帧，结束后标记为非激活
     */
    void Draw() {
        if (!active) return;  // 未激活则不绘制
        if (frame < maxFrame) {  // 未播放完所有帧
            // Modify the line causing the error to explicitly get the raw pointer from the unique_ptr  
            putimage_alpha(pos.x - frames[frame]->getwidth() / 2, pos.y - frames[frame]->getheight() / 2, frames[frame].get());
            frame++;  // 推进到下一帧
        }
        else active = false;  // 播放完所有帧，标记为非激活
    }

    bool IsActive() { return active; }  // 返回动画是否激活
};
unique_ptr<IMAGE> ExplodeAnimation::frames[ExplodeAnimation::maxFrame];

/**
 * 矿石类
 * 功能：表示游戏中的矿石（包括金、石头、钻石等），管理其属性和绘制
 */
class Mine {
public:
    POINT pos;          // 位置
    int type;           // 类型（0=金，1=石头，2=钻石等）
    int value;          // 价值（分数）
    int damage;         // 环境破坏度（采集时增加）
    bool alive = true;  // 是否存在（未被采集或爆炸）
    IMAGE* img;         // 矿石图片
    ExplodeAnimation explodeAnim;  // 爆炸动画
    bool isExploding = false;  // 是否正在爆炸（避免重复触发）
    int pullSpeed;      // 被钩子拉动的速度（值越小越慢）

    // 构造函数：初始化矿石属性
    Mine(int x, int y, int t, int v, int d, IMAGE* i, int speed)
        :pos({ x,y }), type(t), value(v), damage(d), img(i), pullSpeed(speed) {
    }

    /**
     * 绘制矿石
     * 功能：绘制矿石图片，若正在爆炸则绘制爆炸动画
     */
    void Draw() {
        if (alive) {  // 矿石存在时绘制图片
            putimage_alpha(pos.x, pos.y, img);
        }
        if (isExploding) {  // 正在爆炸时绘制动画
            explodeAnim.Draw();
        }
    }
};

/**
 * 处理爆炸范围
 * @param center 爆炸中心
 * @param mines 矿石列表
 * @param envDamage 环境破坏度（引用，用于累加）
 * 功能：检测爆炸范围内的矿石，使其消失并增加破坏度，处理链式爆炸（炸弹触发其他炸弹）
 */
void HandleExplosion(POINT center, vector<Mine*>& mines, int& envDamage) {
    const int EXPLODE_RADIUS = 150;  // 爆炸半径
    for (auto& mine : mines) {
        if (!mine->alive || mine->isExploding) continue;  // 跳过已消失或正在爆炸的矿石

        // 计算矿石中心与爆炸中心的距离
        int dx = mine->pos.x + mine->img->getwidth() / 2 - center.x;
        int dy = mine->pos.y + mine->img->getheight() / 2 - center.y;
        // 用平方距离判断（避免开方，提高效率）
        const int EXPLODE_RADIUS_SQ = EXPLODE_RADIUS * EXPLODE_RADIUS;
        if (dx * dx + dy * dy < EXPLODE_RADIUS_SQ) {
            if (mine->type == 4) {  // 若矿石是炸弹（类型4），触发链式爆炸
                mine->isExploding = true;  // 标记为正在爆炸
                mine->alive = false;       // 矿石消失
                // 以当前炸弹为中心启动爆炸动画
                POINT subCenter = {
                    mine->pos.x + mine->img->getwidth() / 2,
                    mine->pos.y + mine->img->getheight() / 2
                };
                mine->explodeAnim.Start(subCenter);
                envDamage += 10;  // 炸弹爆炸增加10破坏度
                HandleExplosion(subCenter, mines, envDamage);  // 递归处理链式爆炸
            }
            else {  // 非炸弹矿石：直接消失并增加破坏度
                mine->alive = false;
                envDamage += mine->damage;
            }
        }
    }
}

/**
 * 钩子类
 * 功能：管理钩子的状态（摆动、发射、收回）、与矿石的交互（勾取、释放、引爆）
 */
class Hook {
private:
    POINT base{ WIN_WIDTH / 2,80 };  // 钩子基座位置（顶部中间）
    double angle = 0;                // 摆动角度（度）
    int dir = 1;                     // 摆动方向（1=右，-1=左）
    int len = 50;                    // 钩子长度
    int hookState = 0;                   // 状态（0=摆动，1=发射中，2=收回中）
    int speed = 8;                   // 发射/收回速度
    IMAGE* normalImg;                // 普通钩子图片
    IMAGE* catchImg;                 // 勾住矿石时的钩子图片
    Mine* caught = nullptr;          // 勾住的矿石（ nullptr表示未勾住）
    int currentPullSpeed;            // 当前收回速度（受矿石重量影响）
    bool isCatching = false;         // 是否正在勾住矿石
    ExplodeAnimation explodeAnim;    // 爆炸动画
    vector<Mine*>& mines;            // 引用当前关卡的矿石列表（用于爆炸检测）
    GameState& state;  // 引用游戏状态
public:
    // 构造函数：初始化钩子图片和矿石列表引用
    Hook(IMAGE* normal, IMAGE* catchImg, vector<Mine*>& minesRef)
        : state(GameState::Instance()), normalImg(normal), catchImg(catchImg), mines(minesRef) {
    }

    /**
     * 释放勾住的矿石
     * 功能：将勾住的矿石放回场景，重置钩子状态
     */
    void Release() {
        if (hookState == 2 && caught != nullptr) {  // 正在收回且勾住了矿石
            caught->alive = true;  // 矿石重新出现在场景中
            caught = nullptr;      // 钩子不再勾住矿石
            currentPullSpeed = 8;  // 重置收回速度
            isCatching = false;    // 标记未勾住矿石
        }
    }

    /**
     * 引爆炸弹
     * 功能：若勾住的是炸弹，触发爆炸并处理爆炸范围
     */
    void Explode() {
        // 勾住了炸弹且未正在爆炸
        if (caught && caught->type == 4 && !caught->isExploding) {
            mciSendString(_T("play audio\\explode.mp3 from 0"), NULL, 0, NULL);  // 播放爆炸音效
            state.envDamage += 10;  // 增加环境破坏度
            POINT e = GetEnd();  // 获取钩子末端位置（爆炸中心）
            explodeAnim.Start(e);  // 启动爆炸动画

            // 标记炸弹为正在爆炸
            caught->isExploding = true;
            caught->alive = false;
            HandleExplosion(e, mines, state.envDamage);  // 处理爆炸范围内的矿石

            // 重置钩子状态
            caught = nullptr;
            currentPullSpeed = 8;
            isCatching = false;
            hookState = 2;  // 继续收回
        }
    }

    /**
     * 更新钩子状态
     * 功能：根据当前状态（摆动、发射、收回）更新位置和角度
     */
    void Update() {
        if (hookState == 0) {  // 摆动状态
            angle += dir;  // 改变角度（摆动）
            // 到达最大角度时反向
            if (angle > MAX_ANGLE) dir = -1;
            if (angle < -MAX_ANGLE) dir = 1;
        }
        else if (hookState == 1) {  // 发射中
            len += speed;  // 增加钩子长度（向下延伸）
            // 播放发射音效（仅一次）
            if (!state.isLaunchBGMPlaying) {
                mciSendString(_T("play audio\\launch.mp3 from 0"), NULL, 0, NULL);
                state.isLaunchBGMPlaying = 1;
            }
            POINT e = GetEnd();  // 获取钩子末端位置
            // 钩子超出窗口范围则开始收回
            if (e.y > WIN_HEIGHT || e.x < 0 || e.x > WIN_WIDTH) {
                hookState = 2;
                currentPullSpeed = 8;
            }
        }
        else if (hookState == 2) {  // 收回中
            len -= currentPullSpeed;  // 减少钩子长度（向上收回）
            state.isLaunchBGMPlaying = 0;   // 停止发射音效标记

            if (caught) {  // 勾住矿石时，更新矿石位置（随钩子移动）
                POINT hookPos = GetEnd();
                caught->pos.x = hookPos.x - caught->img->getwidth() / 2;  // 矿石中心与钩子对齐
                caught->pos.y = hookPos.y - 10;
                // 特殊矿石的位置微调
                if (caught->type == 0) caught->pos.y += 10;  // 金矿石
                if (caught->type == 2) caught->pos.y += 5;   // 钻石
            }

            // 钩子收回至初始长度
            if (len <= 50) {
                len = 50;  // 重置长度
                if (caught) {  // 勾住矿石且已收回至顶部
                    state.totalScore += caught->value;  // 增加分数
                    state.envDamage += caught->damage;  // 增加环境破坏度

                    // 解锁对应成就
                    if (!state.achieve_first) state.achieve_first = true;  // 首次开采
                    if (caught->type == 3 && !state.achieve_screw) state.achieve_screw = true;  // 食堂螺丝
                    if (caught->type == 4 && !state.achieve_bomb) state.achieve_bomb = true;    // 炸药

                    mciSendString(_T("play audio\\score.mp3 from 0"), NULL, 0, NULL);  // 播放获取音效
                    caught->alive = false;  // 矿石消失
                    caught = nullptr;       // 钩子不再勾住矿石
                    isCatching = false;     // 标记未勾住
                }
                hookState = 0;  // 回到摆动状态
            }
        }
    }

    /**
     * 获取钩子末端位置
     * 功能：根据当前角度和长度计算钩子末端坐标
     */
    POINT GetEnd() {
        double r = angle * 3.1415926 / 180.0;  // 角度转弧度
        // 利用三角函数计算末端位置（基座为起点）
        return {
            base.x + (int)(sin(r) * len),
            base.y + (int)(cos(r) * len)
        };
    }

    /**
     * 绘制钩子
     * 功能：绘制钩子线和钩子图片，若勾住矿石则绘制矿石
     */
    void Draw() {
        POINT e = GetEnd();  // 获取末端位置
        if (caught) {  // 勾住矿石时绘制矿石
            caught->Draw();
        }
        // 绘制钩子线（从基座到末端）
        setlinecolor(BLACK);
        setlinestyle(PS_SOLID, 5);
        line(base.x, base.y, e.x, e.y);
        // 选择钩子图片（勾住/未勾住）
        IMAGE* currentImg = isCatching ? catchImg : normalImg;
        // 计算钩子图片偏移（使图片中心与末端对齐）
        int offsetX = -currentImg->getwidth() / 2;
        int offsetY = -currentImg->getheight() / 2 + 7;
        if (isCatching) offsetY += 5;  // 勾住时微调位置
        // 绘制钩子图片
        putimage_alpha(e.x + offsetX, e.y + offsetY, currentImg);
        // 绘制爆炸动画（若有）
        explodeAnim.Draw();
    }

    /**
     * 发射钩子
     * 功能：从摆动状态切换为发射状态
     */
    void Launch() { if (hookState == 0) hookState = 1; }

    /**
     * 检测是否勾住矿石
     * @param m 矿石对象
     * 功能：判断钩子末端是否与矿石碰撞，若是则勾住矿石
     */
    bool Hooking(Mine* m) {
        if (caught) return false;  // 已勾住矿石则返回
        POINT e = GetEnd();  // 钩子末端位置
        // 矿石宽高的一半（用于碰撞检测）
        int mw = m->img->getwidth(), mh = m->img->getheight();
        // 检测钩子末端是否在矿石范围内（简单碰撞检测）
        if (m->alive && abs(e.x - (m->pos.x + mw / 2)) < mw / 2 && abs(e.y - (m->pos.y + mh / 2)) < mh / 2) {
            caught = m;                // 勾住矿石
            currentPullSpeed = m->pullSpeed;  // 设定收回速度（矿石越重速度越慢）
            mciSendString(_T("play audio\\hit.mp3 from 0"), NULL, 0, NULL);  // 播放碰撞音效
            hookState = 2;                 // 切换为收回状态
            isCatching = true;         // 标记正在勾住
            return true;
        }
        return false;
    }

    /**
     * 判断钩子是否正在收回
     * 功能：用于控制矿石生成时机（收回时不生成新矿石）
     */
    bool Retracting() { return hookState == 2; }
};

/**
 * 采矿游戏类
 * 功能：管理单个关卡的游戏逻辑（矿石生成、更新、绘制、输入处理）
 */
class MinerGame {
private:
    vector<Mine*> mines;  // 矿石列表
    Hook* hook;           // 钩子对象
    IMAGE* bg;            // 关卡背景图
    ULONGLONG lastTimeCheck;  // 上一次检查时间（用于倒计时）
    GameState& state;  // 引用游戏状态
    int level;            // 当前关卡号
    bool levelOver = false;  // 单个关卡是否结束

public:
    /**
     * 构造函数
     * @param b 背景图
     * @param normalHook 普通钩子图片
     * @param catchHook 勾住时的钩子图片
     * @param imgs 矿石图片列表
     * @param level 关卡号
     * 功能：初始化关卡（生成矿石、钩子）
     */
    MinerGame(IMAGE* b, IMAGE* normalHook, IMAGE* catchHook, vector<IMAGE*> imgs, int level) : bg(b), state(GameState::Instance()), level(level) {
        hook = new Hook(normalHook, catchHook, mines);  // 创建钩子（传入矿石列表引用）
        srand((unsigned)time(nullptr));  // 随机数种子
        lastTimeCheck = GetTickCount64();  // 初始化倒计时检查时间
        // 设置当前关卡的时间
        state.gameTime = state.levelTimes[level - 1];
        // 设置当前关卡的基础掉分
        state.baseScoreReduction = level * 4;
        // 各关卡矿石数量（根据关卡难度调整）
        int countGold = 0, countStone = 0, countDiamond = 0, countScrew = 0, countBomb = 0, countCoal = 0, countIron = 0;
        if (level == 1) {  // 第一关：基础矿石
            countStone = 4; countBomb = 2; countScrew = 6; countGold = 0; countCoal = 3; countIron = 0;
        }
        else if (level == 2) {  // 第二关：增加价值较高的矿石
            countStone = 4; countGold = 2; countScrew = 4; countDiamond = 0; countBomb = 4; countCoal = 4; countIron = 2;
        }
        else if (level == 3) {  // 第三关：增加钻石和铁矿
            countStone = 6; countGold = 3; countDiamond = 2; countBomb = 8; countCoal = 2; countIron = 4; countScrew = 0;
        }
        else if (level == 4) {  // 第四关：高价值矿石为主
            countStone = 6; countGold = 4; countDiamond = 3; countBomb = 8; countCoal = 0; countIron = 2; countScrew = 0;
        }
        else if (level == 5) {  // 第五关：最高难度
            countStone = 8; countGold = 6; countDiamond = 4; countBomb = 8; countCoal = 0; countIron = 0; countScrew = 0;
        }

        // 钩子基座位置（用于矿石生成的安全距离计算）
        POINT hookBase = { WIN_WIDTH / 2, 80 };
        // 安全距离：离钩子至少150像素，矿石之间至少64像素
        const int SAFE_DISTANCE_TO_HOOK = 150;
        const int SAFE_DISTANCE_BETWEEN_MINES = 64;

        // 矿石生成函数（lambda表达式，简化重复代码）
        auto spawn = [&](int type, int count) {
            for (int i = 0; i < count; i++) {
                int x, y;
                bool ok;
                do {
                    ok = true;
                    // 随机生成位置（窗口范围内）
                    x = rand() % (WIN_WIDTH - 64);
                    y = rand() % (WIN_HEIGHT - 200) + 150;

                    // 检查是否离钩子太近
                    if (sqrt(pow(x - hookBase.x, 2) + pow(y - hookBase.y, 2)) < SAFE_DISTANCE_TO_HOOK) {
                        ok = false;
                        continue;
                    }

                    // 检查是否与其他矿石太近
                    for (auto m : mines) {
                        if (sqrt(pow(x - m->pos.x, 2) + pow(y - m->pos.y, 2)) < SAFE_DISTANCE_BETWEEN_MINES) {
                            ok = false;
                            break;
                        }
                    }
                } while (!ok);  // 直到生成合法位置

                // 根据矿石类型设置属性（价值、破坏度、收回速度）
                int v, d, speed;
                switch (type) {
                case 0: v = 500; d = 3; speed = 4; break;  // 金
                case 1: v = 50; d = 5; speed = 2; break;   // 石头
                case 2: v = 1000; d = 2; speed = 8; break; // 钻石
                case 3: v = 1; d = 1; speed = 8; break;    // 螺丝
                case 4: v = 0; d = 1; speed = 7; break;    // 炸弹
                case 5: v = 200; d = 3; speed = 4; break;  // 煤矿
                case 6: v = 300; d = 3; speed = 5; break;  // 铁矿
                }
                // 添加矿石到列表
                mines.push_back(new Mine(x, y, type, v, d, imgs[type], speed));
            }
            };

        // 生成各类型矿石
        spawn(0, countGold);
        spawn(1, countStone);
        spawn(2, countDiamond);
        spawn(3, countScrew);
        spawn(4, countBomb);
        spawn(5, countCoal);
        spawn(6, countIron);
    }

    /**
     * 析构函数
     * 功能：释放钩子和矿石的内存
     */
    ~MinerGame() {
        delete hook;
        for (auto m : mines) delete m;  // 释放所有矿石
    }

    /**
     * 更新游戏状态
     * 功能：更新倒计时、钩子状态、矿石碰撞检测
     */
    void Update() {
        // 倒计时逻辑（每秒减少1）
        ULONGLONG now = GetTickCount64();
        if (now - lastTimeCheck >= 1000 && !state.gameOver) {  // 间隔1秒
            state.gameTime--;
            lastTimeCheck = now;
            if(state.totalScore>3000)
            state.totalScore = max(0, state.totalScore - state.baseScoreReduction);
            // 最后10秒播放提示音效
            if (state.gameTime <= 10 && state.gameTime > 0 && !state.isTimeupBGMPlaying) {
                state.isTimeupBGMPlaying = 1;
                mciSendString(_T("play audio\\last-10s-sound.mp3 from 0"), NULL, 0, NULL);
            }
            // 时间结束，关卡结束
            if (state.gameTime <= 0) {
                state.isTimeupBGMPlaying = 0;
                levelOver = true;
            }
        }

        if (!levelOver) {  // 关卡未结束则更新
            hook->Update();  // 更新钩子
            if (hook->Retracting() || state.gameOver) return;  // 收回时不检测碰撞
            // 检测钩子是否勾住矿石
            for (auto m : mines) if (hook->Hooking(m)) break;
        }
    }

    /**
     * 绘制游戏画面
     * 功能：绘制背景、矿石、钩子、分数、时间、破坏度，处理关卡结束和结局
     */
    void Draw() {
        // 绘制背景
        StretchBlt(GetImageHDC(NULL), 0, 0, WIN_WIDTH, WIN_HEIGHT,
            GetImageHDC(bg), 0, 0, bg->getwidth(), bg->getheight(), SRCCOPY);

        // 绘制矿石
        for (auto m : mines) {
            if (m->alive) m->Draw();
        }
        // 绘制爆炸动画（确保在矿石上方）
        for (auto m : mines) {
            if (m->isExploding) m->explodeAnim.Draw();
        }

        // 绘制钩子
        hook->Draw();

        // 显示分数、时间、环境破坏度
        TCHAR buf[64];
        settextcolor(BLACK);
        settextstyle(30, 0, _T("微软雅黑"));

        _stprintf_s(buf, _T("分数：%d"), state.totalScore);
        outtextxy(20, 20, buf);  // 左上角

        _stprintf_s(buf, _T("时间：%d 秒"), state.gameTime);
        outtextxy(20, 60, buf);

        _stprintf_s(buf, _T("环境破坏度：%d"), state.envDamage);
        outtextxy(20, 100, buf);

        // 关卡结束处理
        if (levelOver) {
            mciSendString(_T("stop mining"), NULL, 0, NULL);  // 停止关卡音乐
            ULONGLONG showStart = GetTickCount64();
            const int showDuration = 3000;  // 显示3秒关卡结束提示

            mciSendString(_T("play audio\\timeup.mp3 from 0"), NULL, 0, NULL);  // 播放时间到音效

            // 显示关卡结束提示
            while (GetTickCount64() - showStart < showDuration) {
                StretchBlt(GetImageHDC(NULL), 0, 0, WIN_WIDTH, WIN_HEIGHT,
                    GetImageHDC(bg), 0, 0, bg->getwidth(), bg->getheight(), SRCCOPY);
                for (auto m : mines) if (m->alive) m->Draw();
                hook->Draw();

                // 显示提示文字
                setbkmode(TRANSPARENT);
                settextstyle(80, 0, _T("微软雅黑"));
                settextcolor(RED);
                const TCHAR* text = (level < state.MAX_LEVEL) ?
                    _T("Time is up!\n即将进入更深层探索......") :  // 非最后一关
                    _T("Time is up!\n游戏结束!");  // 最后一关
                RECT r = { 0, WIN_HEIGHT / 3, WIN_WIDTH, WIN_HEIGHT };
                drawtext(text, &r, DT_CENTER | DT_VCENTER | DT_WORDBREAK);  // 居中显示
                FlushBatchDraw();
                Sleep(16);
            }

            // 最后一关结束，判定结局
            if (level == state.MAX_LEVEL) {
                state.gameOver = 1;  // 标记游戏结束
                mciSendString(_T("play end repeat from 0"), NULL, 0, NULL);  // 播放结局音乐

                // 根据分数和破坏度判定结局
                const TCHAR* resultText;
                const TCHAR* desc;
                IMAGE* endingImg;

                if (state.totalScore < 5000 && state.envDamage < 100) {  // 结局1：开采不足
                    state.achieve_underdev = true;
                    endingImg = &ending1;
                    resultText = _T("结局1：采集不足");
                    desc = _T("求小实的资源采集量未达到工程要求，泥开建设进度滞后。\n最终未赶上工期，二期仍是一片草坪，学校排名日渐降低...\n提示：在不过度破坏大黑山的情况下优先选择高价值矿石吧!");
                }
                else if (state.totalScore >= 5000 && state.envDamage < 100) {  // 结局2：完美开发
                    state.achieve_perfectdev = true;
                    endingImg = &ending2;
                    resultText = _T("结局2：完美开采");
                    desc = _T("求小实完美平衡了开发与生态，泥开顺利扩建。\n学校排名直升，跻身中九，剑指C9。\n这就是泥开与大黑山二十多年的羁绊啊！");
                }
                else {  // 结局3：开发过度
                    state.achieve_overdev = true;
                    endingImg = &ending3;
                    resultText = _T("结局3：开发过度");
                    desc = _T("求小实过度破坏生态，泥开虽顺利扩建,但环境已经不适宜学生生存,最终沦为惨不忍睹的荒地,学校排名直线下降。\n多年以后，当求小实再次回到泥开，将会想起当年开发大黑山的那个下午...");
                }

                // 绘制结局画面
                StretchBlt(GetImageHDC(NULL), 0, 0, WIN_WIDTH, WIN_HEIGHT,
                    GetImageHDC(endingImg), 0, 0, endingImg->getwidth(), endingImg->getheight(), SRCCOPY);

                // 显示结局标题
                settextstyle(60, 0, _T("微软雅黑"));
                settextcolor(RED);
                SIZE sz;
                GetTextExtentPoint32(GetImageHDC(), resultText, _tcslen(resultText), &sz);
                outtextxy((WIN_WIDTH - sz.cx) / 2, WIN_HEIGHT / 4 - 50, resultText);

                // 显示结局描述
                settextstyle(40, 0, _T("微软雅黑"), 0, 0, 700, false, false, false);
                settextcolor(RGB(255, 20, 147));
                RECT descRect = { 150, WIN_HEIGHT / 4 + 25, WIN_WIDTH - 100, WIN_HEIGHT / 2 + 100 };
                drawtext(desc, &descRect, DT_CENTER | DT_WORDBREAK);

                // 绘制返回菜单按钮
                state.showReturnBtn = true;
                setfillcolor(RGB(100, 200, 100));
                fillrectangle(returnBtn.left, returnBtn.top, returnBtn.right, returnBtn.bottom);
                settextstyle(40, 0, _T("微软雅黑"));
                settextcolor(BLACK);
                outtextxy(returnBtn.left + 20, returnBtn.top + 10, _T("  返回菜单"));
                FlushBatchDraw();
            }
        }
    }

    /**
     * 处理输入事件
     * @param msg 消息
     * 功能：响应鼠标和键盘输入（发射钩子、释放矿石、引爆炸弹）
     */
    void Process(const ExMessage& msg) {
        if (msg.message == WM_LBUTTONDOWN && !state.gameOver)
            hook->Launch();  // 左键发射钩子

        if (msg.message == WM_RBUTTONDOWN && !state.gameOver)
            hook->Release();  // 右键释放矿石

        if (msg.message == WM_KEYDOWN && msg.vkcode == VK_SPACE && !state.gameOver)
            hook->Explode();  // 空格引爆炸弹
    }

    /**
     * 判断关卡是否结束
     */
    bool IsLevelOver() {
        return levelOver;
    }
};

/**
 * 统计当前解锁的成就数量
 * 功能：用于判断是否获得新成就（与游戏前的数量对比）
 */
int GetCurrentAchieveCount() {
    int count = 0;
    GameState& state = GameState::Instance();
    count += state.achieve_first;
    count += state.achieve_screw;
    count += state.achieve_bomb;
    count += state.achieve_underdev;
    count += state.achieve_perfectdev;
    count += state.achieve_overdev;
    return count;
}

// 全局变量初始化
vector<IMAGE*> imgs{ &goldImg, &rockImg, &diamondImg, &screwImg, &bombImg, &coalImg, &ironImg };
Menu menu(&menuBG);

/**
 * 主函数
 * 功能：程序入口，初始化资源、主循环（菜单与游戏切换）、关卡流程控制
 */
int main() {
    initgraph(WIN_WIDTH, WIN_HEIGHT);  // 初始化图形窗口
    HWND hWnd = GetHWnd();
    SetWindowText(hWnd, _T("大黑山的诱惑"));  // 设置窗口标题
    BeginBatchDraw();  // 开始批量绘制（提高效率）

    // 打开背景音乐资源（预加载，避免播放时卡顿）
    mciSendString(_T("open audio\\main_menu_bgm.mp3 alias menu"), NULL, 0, NULL);
    mciSendString(_T("open audio\\cut_scene.mp3 alias cutscene"), NULL, 0, NULL);
    mciSendString(_T("open audio\\instruction_bgm.mp3 alias instruction"), NULL, 0, NULL);
    mciSendString(_T("open audio\\ending_bgm.mp3 alias end"), NULL, 0, NULL);
    mciSendString(_T("open audio\\mining_bgm.mp3 alias mining"), NULL, 0, NULL);

    // 加载图片资源
    loadimage(&cutSceneImg, _T("image\\cut_scene.png"));
    loadimage(&menuBG, _T("image\\main_menu_bg.png"));
    loadimage(&instructionBG, _T("image\\instruction_bg.png"));
    loadimage(&mineBG, _T("image\\mining_bg.png"));
    loadimage(&qiuxiaoshiImg, _T("image\\qiuxiaoshi.png"));
    loadimage(&marryNewImg, _T("image\\marry_new.png"));
    loadimage(&hookImg, _T("image\\hook.png"));
    loadimage(&catchImg, _T("image\\catch.png"));
    loadimage(&goldImg, _T("image\\gold.png"));
    loadimage(&rockImg, _T("image\\rock.png"));
    loadimage(&coalImg, _T("image\\coal.png"));
    loadimage(&ironImg, _T("image\\iron.png"));
    loadimage(&diamondImg, _T("image\\diamond.png"));
    loadimage(&screwImg, _T("image\\screw.png"));
    loadimage(&bombImg, _T("image\\bomb.png"));
    loadimage(&achieve_first_img, _T("image\\achieve_first.png"));
    loadimage(&achieve_screw_img, _T("image\\achieve_screw.png"));
    loadimage(&achieve_bomb_img, _T("image\\achieve_bomb.png"));
    loadimage(&achieve_underdev_img, _T("image\\achieve_underdevelopment.png"));
    loadimage(&achieve_perfectdev_img, _T("image\\achieve_perfect_development.png"));
    loadimage(&achieve_overdev_img, _T("image\\achieve_overdevelopment.png"));
    loadimage(&ending1, _T("image\\ending1.png"));
    loadimage(&ending2, _T("image\\ending2.png"));
    loadimage(&ending3, _T("image\\ending3.png"));
    loadimage(&achieveBG, _T("image\\achieve_bg.png"));
    loadimage(&lockedImg, _T("image\\locked.png"));

    ExplodeAnimation::LoadResources();  // 加载爆炸动画帧

    setlinecolor(BLACK);
    setlinestyle(PS_SOLID, 5);

    // 主循环
    while (GameState::Instance().isRunning) {
        ExMessage msg;
        while (peekmessage(&msg)) {
            if (msg.message == WM_CLOSE) {  // 用户点击关闭按钮
                GameState::Instance().isRunning = false;  // 标记退出循环
                break;
            }
        }
        if (!GameState::Instance().isRunning) break;  // 收到关闭事件，退出循环
        if (GameState::Instance().isInMenu) {  // 在主菜单
            int menuResult = menu.Run();
            if (menuResult == 1) {  // 点击开始游戏
                GameState& state = GameState::Instance();
                state.lastAchieveCount = GetCurrentAchieveCount();  // 记录当前成就数

                // 初始化游戏状态
                state.isInMenu = false;
                state.totalScore = 0;
                state.envDamage = 0;
                state.currentLevel = 1;
                state.gameOver = false;
                state.showReturnBtn = false;
                ExMessage msg;

                mciSendString(_T("stop menu"), NULL, 0, NULL);  // 停止菜单音乐

                // 关卡流程（从1到MAX_LEVEL）
                while (state.currentLevel <= state.MAX_LEVEL) {
                    ShowCutScene(state.currentLevel);  // 播放过场动画
                    mciSendString(_T("play mining repeat from 0"), NULL, 0, NULL);  // 播放关卡音乐

                    // 设置当前关卡时间（随关卡增加）
                    state.gameTime = 80 - (state.currentLevel - 1) * 10;
                    MinerGame game(&mineBG, &hookImg, &catchImg, imgs, state.currentLevel);  // 创建关卡

                    // 单个关卡的游戏循环
                    while (!game.IsLevelOver()) {
                        ULONGLONG t0 = GetTickCount64();  // 记录当前时间

                        // 处理输入
                        while (peekmessage(&msg)) {
                            game.Process(msg);
                        }

                        game.Update();  // 更新游戏状态
                        game.Draw();    // 绘制游戏画面
                        FlushBatchDraw();  // 刷新

                        // 控制帧率（约60帧）
                        ULONGLONG t1 = GetTickCount64();
                        if (t1 - t0 < 1000 / GAME_FRAME)
                            Sleep(1000 / GAME_FRAME - (t1 - t0));
                    }
                    state.currentLevel++;  // 进入下一关
                }

                // 游戏结束后，等待返回菜单
                while (!state.isInMenu) {
                    while (peekmessage(&msg)) {
                        if (msg.message == WM_LBUTTONDOWN) {  // 点击返回按钮
                            int x = msg.x, y = msg.y;
                            if (PtInRect(&returnBtn, { x, y })) {
                                state.isInMenu = true;  // 返回菜单
                                break;
                            }
                        }
                    }
                }

                // 检查是否获得新成就
                state.gotNewAchievement = GetCurrentAchieveCount() > state.lastAchieveCount;
                // 更新最高分
                if (state.totalScore > state.tempHighScore) {
                    state.tempHighScore = state.totalScore;
                }

                // 重置状态
                mciSendString(_T("stop end"), NULL, 0, NULL);
                state.currentLevel = 1;
                state.totalScore = 0;
                state.envDamage = 0;
                state.gameOver = false;
                state.isMenuBGMPlaying = false;
                FlushBatchDraw();
            }
        }
    }

    mciSendString(_T("close menu"), NULL, 0, NULL);
    mciSendString(_T("close cutscene"), NULL, 0, NULL);
    mciSendString(_T("close instruction"), NULL, 0, NULL);
    mciSendString(_T("close end"), NULL, 0, NULL);
    mciSendString(_T("close mining"), NULL, 0, NULL);

    EndBatchDraw();
    closegraph();
    return 0;
}