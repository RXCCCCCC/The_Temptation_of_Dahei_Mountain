// ���ɽ���ջ�
// ����ͼ�ο⡢Windows API����ý���ͷ�ļ�
#include <windows.h>
#include <graphics.h>    // EasyXͼ�ο⣬���ڻ�ͼ
#include <mmsystem.h>    // ��ý��ӿڣ����ڲ�������
#include <vector>        // ��̬���飬�洢��ʯ����
#include <string>
#include <cmath>         // ��ѧ���������ڼ�����롢�Ƕ�
#include <ctime>         // ʱ�亯�����������������
#include <memory> 
#pragma comment(lib, "MSIMG32.LIB")  // ����͸����������Ŀ�
#pragma comment(lib, "Winmm.LIB")    // ���Ӷ�ý���
using namespace std;

// ���ڳ�������
const int WIN_WIDTH = 1080;    // ���ڿ��
const int WIN_HEIGHT = 640;    // ���ڸ߶�
const int GAME_FRAME = 60;     // ��Ϸ֡�ʣ�ÿ��60֡��
const int MAX_ANGLE = 80;      // �������ڶ��Ƕȣ��ȣ�
class GameState {
public:
    // ��Ϸ����״̬
    bool isInMenu = true;
    bool gameOver = false;
    bool showReturnBtn = false;
    bool isInInstruction = false;
    bool isPlayingCutScene = false;
    bool isRunning = true;

    // ��������״̬
    bool isMenuBGMPlaying = false;
    bool isInstructionBGMPlaying = false;
    bool isLaunchBGMPlaying = false;
    bool isPullBGMPlaying = false;
    bool isTimeupBGMPlaying = false;

    // �˵���ɾ����
    bool achieve_first = false;
    bool achieve_screw = false;
    bool achieve_bomb = false;
    bool achieve_underdev = false;
    bool achieve_perfectdev = false;
    bool achieve_overdev = false;
    bool gotNewAchievement = false;
    int lastAchieveCount = 0;

    // ��Ϸ����
    int tempHighScore = 0;
    int totalScore = 0;
    int gameTime = 40;
    int envDamage = 0;
    int currentLevel = 1;
    const int MAX_LEVEL = 5;
    int levelTimes[5] = { 40, 50, 60, 50, 40 };
    int baseScoreReduction = 0;

    // ����ģʽʵ��
    static GameState& Instance() {
        static GameState instance;
        return instance;
    }

private:
    GameState() = default;  // ˽�й��캯����ȷ������
    GameState(const GameState&) = delete;
    GameState& operator=(const GameState&) = delete;
};

// ��ťλ�ã���������
RECT returnBtn{ 440, 400, 640, 460 }; // ���ذ�ťλ�ã����У�

// ͼƬ��Դ��ȫ���������������أ�
IMAGE menuBG, mineBG, instructionBG, cutSceneImg, achieveBG, qiuxiaoshiImg, marryNewImg;
IMAGE hookImg, catchImg, goldImg, rockImg, diamondImg, screwImg, bombImg, coalImg, ironImg;
IMAGE ending1, ending2, ending3, achieve_first_img, achieve_screw_img, achieve_bomb_img,
achieve_underdev_img, achieve_perfectdev_img, achieve_overdev_img, lockedImg;

/**
 * ͸������ͼƬ
 * @param x ����λ��x����
 * @param y ����λ��y����
 * @param img Ҫ���Ƶ�ͼƬ
 * ���ܣ�ʹ��AlphaBlendʵ��ͼƬ��͸��Ч�������ⱳ���ڵ�
 */
void putimage_alpha(int x, int y, IMAGE* img) {
    int w = img->getwidth();   // ��ȡͼƬ���
    int h = img->getheight();  // ��ȡͼƬ�߶�
    HDC dstDC = GetImageHDC(NULL);  // Ŀ���豸�����ģ����ڣ�
    HDC srcDC = GetImageHDC(img);   // Դ�豸�����ģ�ͼƬ��
    // ͸����ϲ�����ʹ��ԴͼƬ��Alphaͨ��
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    // ִ��͸������
    AlphaBlend(dstDC, x, y, w, h, srcDC, 0, 0, w, h, bf);
}

/**
 * ��ʾ�ؿ���������
 * @param level �ؿ���
 * ���ܣ��ڹؿ���ʼǰ��ʾ��������ʾ��ǰ�ؿ�������3��
 */
void ShowCutScene(int level) {
    GameState& state = GameState::Instance();
    state.isPlayingCutScene = true;               // ������ڲ��Ź���
    ULONGLONG startTime = GetTickCount64(); // ��¼��ʼʱ��
    const int CUTSCENE_DURATION = 3000;     // ����ʱ��3000���루3�룩

    // ���Ź�������
    mciSendString(_T("play cutscene from 0"), NULL, 0, NULL);

    // ѭ����ʾ����������ֱ��ʱ�����
    while (GetTickCount64() - startTime < CUTSCENE_DURATION) {
        ExMessage msg;
        while (peekmessage(&msg)) {};  // �����Ϣ���У����⽻��

        // ���ƹ�������ͼ���������䴰�ڣ�
        StretchBlt(GetImageHDC(NULL), 0, 0, WIN_WIDTH, WIN_HEIGHT,
            GetImageHDC(&cutSceneImg), 0, 0,
            cutSceneImg.getwidth(), cutSceneImg.getheight(), SRCCOPY);

        // ��ʾ�ؿ����֣��硰��һ�ء���
        setbkmode(TRANSPARENT);         // ���ֱ���͸��
        settextstyle(60, 0, _T("΢���ź�")); // ������ʽ
        settextcolor(WHITE);            // ������ɫ��ɫ
        TCHAR levelText[32];
        _stprintf_s(levelText, _T("��%d��"), level);  // ��ʽ���ؿ�����
        SIZE sz;
        GetTextExtentPoint32(GetImageHDC(), levelText, _tcslen(levelText), &sz); // ��ȡ���ֳߴ�
        // ������ʾ�ؿ�����
        outtextxy((WIN_WIDTH - sz.cx) / 2, WIN_HEIGHT / 2 - 30, levelText);

        FlushBatchDraw();  // ˢ�»���
        Sleep(16);         // ����֡�ʣ�Լ60֡��
    }

    // ������������
    mciSendString(_T("stop cutscene"), NULL, 0, NULL);  // ֹͣ����
    state.isPlayingCutScene = false;  // ��ǹ�������
}

/**
 * �ɾͲ˵���
 * ���ܣ�����ɾ͵���ʾ������״̬�����ز˵�����
 */
class AchieveMenu {
public:
    IMAGE* bg;                  // �ɾͽ��汳��ͼ
    RECT backBtn{ 830, 50, 1020, 110 };  // ���ذ�ťλ��
    GameState& state;  // ������Ϸ״̬
    // ���캯������ʼ������ͼ
    AchieveMenu(IMAGE* bgImg) : bg(bgImg), state(GameState::Instance()) {}

    // �ɾ���Ϣ�ṹ�壺�洢�����ɾ͵�״̬����ʾ����
    struct AchieveInfo {
        bool unlocked;          // �Ƿ����
        IMAGE* unlockedImg;     // �������ͼƬ
        const TCHAR* title;     // �ɾͱ���
        const TCHAR* desc;      // �ɾ�����
    };

    /**
     * ���Ƶ����ɾ�
     * @param x ����x����
     * @param y ����y����
     * @param info �ɾ���Ϣ
     * ���ܣ����ݽ���״̬���Ƴɾͣ�������ʾͼƬ�����֣�δ������ʾ��ͼ�꣩
     */
    void DrawSingleAchieve(int x, int y, const AchieveInfo& info) {
        // ѡ����ʾ��ͼƬ������/δ������
        IMAGE* img = info.unlocked ? info.unlockedImg : &lockedImg;
        // ѡ����ʾ�����֣�����/δ������
        const TCHAR* title = info.unlocked ? info.title : _T("???");
        const TCHAR* desc = info.unlocked ? info.desc : _T("???");

        // ���Ƴɾ�ͼƬ
        putimage_alpha(x, y, img);

        // ���Ƴɾͱ��������
        setbkmode(TRANSPARENT);
        settextcolor(BLACK);  // ������ɫ��ɫ

        // ���Ʊ���
        settextstyle(30, 0, _T("΢���ź�"));
        outtextxy(x + 90, y, title);  // ������ͼƬ�Ҳ�

        // �����������Զ����У�
        settextstyle(24, 0, _T("΢���ź�"));
        RECT descRect = { x + 90, y + 35, x + 1200, y + 100 };  // ��������
        drawtext(desc, &descRect, DT_WORDBREAK);  // �Զ����л���
    }

    /**
     * ���гɾͲ˵�
     * ���ܣ�ѭ����ʾ�ɾͽ��棬�����ذ�ť���
     */
    void Run() {
        ExMessage msg;  // ��Ϣ����

        // �������гɾ͵���Ϣ��˳����ɾͱ�����Ӧ��
        AchieveInfo achieves[] = {
            {state.achieve_first, &achieve_first_img, _T("�״ο���"), _T("Ϊ���࿪!")},
            {state.achieve_screw, &achieve_screw_img, _T("ʳ����˿"), _T("��˵������ʳ�ñ���������˿,�ѵ��Ǳ��紵�ϴ��ɽ��?(bushi")},
            {state.achieve_bomb, &achieve_bomb_img, _T("���ǣ�ըҩ��"), _T("��˵�����࿪���ڿ���ʹ���˴���ըҩ,������ܱ��ھ�Ͷ����...���������Ҳ��Ҫ��,�ܲ��þͱ��ð�!")},
            {state.achieve_underdev, &achieve_underdev_img, _T("���ɲ���"), _T("�����ô��Ժ�����࿪����?")},
            {state.achieve_perfectdev, &achieve_perfectdev_img, _T("��������"), _T("�����࿪�ٹ�,�ұ��岻�ݴ�!")},
            {state.achieve_overdev, &achieve_overdev_img, _T("��������"), _T("��ҪС���࿪�����ɽ���!")},
        };

        const int lineSpacing = 90;  // �ɾ�֮����о�
        const int startX = 100, startY = 80;  // �׸��ɾ͵���ʼλ��

        // �ɾͽ�����ѭ��
        while (true) {
            // ���Ʊ���ͼ
            StretchBlt(GetImageHDC(NULL), 0, 0, WIN_WIDTH, WIN_HEIGHT,
                GetImageHDC(bg), 0, 0, bg->getwidth(), bg->getheight(), SRCCOPY);

            // ����ÿ���ɾ�
            for (int i = 0; i < 6; i++) {
                int y = startY + i * lineSpacing;  // �����i���ɾ͵�y����
                DrawSingleAchieve(startX, y, achieves[i]);
            }

            // ���Ʒ��ذ�ť
            setfillcolor(RGB(100, 200, 100));  // ��ť��ɫ��ǳ��ɫ��
            fillrectangle(backBtn.left, backBtn.top, backBtn.right, backBtn.bottom);  // ���ư�ť����
            settextcolor(BLACK);
            settextstyle(40, 0, _T("΢���ź�"));
            outtextxy(backBtn.left + 20, backBtn.top + 10, _T("  ���ز˵�"));  // ��ť����

            FlushBatchDraw();  // ˢ�»���

            // �������¼�
            while (peekmessage(&msg)) {
                if (msg.message == WM_LBUTTONDOWN) {  // ������
                    // ����Ƿ������ذ�ť
                    if (PtInRect(&backBtn, { msg.x, msg.y })) return;  // �˳��ɾͽ���
                }
            }
        }
    }
};
AchieveMenu achieveMenu(&achieveBG);  // ʵ�����ɾͲ˵�

/**
 * ���˵���
 * ���ܣ��������˵����棨��ʼ��Ϸ���������ɾͰ�ť���������л�
 */
class Menu {
public:
    IMAGE* bg;  // �˵�����ͼ
    GameState& state;
    // ��ťλ�ã���ʼ��Ϸ���������ɾͣ�
    RECT btn_start{ 390,200,690,270 }, btn_help{ 390,300,690,370 }, btn_achieve{ 390,400,690,470 };

    // ���캯������ʼ������ͼ
    Menu(IMAGE* bgImg) : bg(bgImg) , state(GameState::Instance() ){}

    /**
     * ���в˵�
     * ���ܣ�ѭ����ʾ�˵�������ť��������ز��������1=��ʼ��Ϸ��0=������
     */
    int Run() {
        ExMessage msg;  // ��Ϣ����
        while (true) {
            settextstyle(32, 0, _T("΢���ź�"), 0, 0, 700, 0, 0, 0);

            // �������Ϸ˵������
            if (state.isInInstruction) {
                // ����˵�����汳�����֣����״ν���ʱ��
                if (!state.isInstructionBGMPlaying) {
                    mciSendString(_T("stop menu"), NULL, 0, NULL);  // ֹͣ�˵�����
                    mciSendString(_T("play instruction repeat from 0"), NULL, 0, NULL);  // ����˵������
                    state.isInstructionBGMPlaying = true;
                    state.isMenuBGMPlaying = false;
                }

                // ����˵�����汳��
                StretchBlt(GetImageHDC(NULL), 0, 0, WIN_WIDTH, WIN_HEIGHT,
                    GetImageHDC(&instructionBG), 0, 0,
                    instructionBG.getwidth(), instructionBG.getheight(), SRCCOPY);

                // �����淨˵������
                setbkmode(TRANSPARENT);
                settextcolor(YELLOW);
                settextstyle(32, 0, _T("΢���ź�"), 0, 0, 700, 0, 0, 0);
                RECT textRect = { 100, 50, WIN_WIDTH - 100, WIN_HEIGHT - 100 };  // ��������
                const TCHAR* text = _T("�淨˵����������乳��,��ȡ����ʯʱ�����Ҽ�����,��ȡ��ըҩʱ���԰��ո�����\n������: �����ѧ������У��һֱ�����������ѧ�����ߵĺ���(bushi\n���Ǻ�,Ϊ������һ��ѧ��,�࿪������������ing...\nΪ�������࿪��ʩ��,marry new�޷�����ס���ɽ���ջ�,������ǲ��Сʵȥ�������ɽ�Ŀ����Դ\n���ܹ����������?\n��������: Ҫ�ڹ�5000��,����̫����Сʵ������ڿ�ɱ�,���ķ���,��Խ�������Խ��Ŷ��ע�⻷��,��Ҫ�����࿪�ʹ��ɽ��ʮ������!\n��������: �����鴿���鹹,������ͬ,�����ɺ�");
                drawtext(text, &textRect, DT_WORDBREAK);  // �Զ�����

                // ���Ʒ��ز˵���ť
                setfillcolor(RGB(100, 200, 100));  // ��ť��ɫ
                fillrectangle(returnBtn.left, returnBtn.top, returnBtn.right, returnBtn.bottom);
                settextstyle(40, 0, _T("΢���ź�"));
                settextcolor(BLACK);
                outtextxy(returnBtn.left + 20, returnBtn.top + 10, _T("  ���ز˵�"));

                // �������½ǽ�ɫͼƬ����Сʵ��
                int leftBottomX = 150;
                int leftBottomY = WIN_HEIGHT - qiuxiaoshiImg.getheight();
                putimage_alpha(leftBottomX, leftBottomY, &qiuxiaoshiImg);

                // �������½ǽ�ɫͼƬ��marry new��
                int rightBottomX = WIN_WIDTH - marryNewImg.getwidth();
                int rightBottomY = WIN_HEIGHT - marryNewImg.getheight();
                putimage_alpha(rightBottomX, rightBottomY, &marryNewImg);

                FlushBatchDraw();  // ˢ�»���

                // ����˵������ĵ���¼�
                while (peekmessage(&msg)) {
                    if (msg.message == WM_LBUTTONDOWN) {  // ������
                        int x = msg.x, y = msg.y;
                        // ������ذ�ť���ص����˵�
                        if (PtInRect(&returnBtn, { x, y })) {
                            mciSendString(_T("stop instruction"), NULL, 0, NULL);  // ֹͣ˵������
                            state.isInInstruction = false;  // ����˳�˵������
                            return 0;  // ���ز˵���ѭ��
                        }
                    }
                }
            }
            // ���˵�����
            else {
                // ���Ų˵��������֣����״ν���ʱ��
                if (!state.isMenuBGMPlaying) {
                    mciSendString(_T("play menu repeat from 0"), NULL, 0, NULL);  // ���Ų˵�����
                    state.isMenuBGMPlaying = true;
                    state.isInstructionBGMPlaying = false;
                }

                // ���Ʋ˵�����
                StretchBlt(GetImageHDC(NULL), 0, 0, WIN_WIDTH, WIN_HEIGHT, GetImageHDC(bg), 0, 0, bg->getwidth(), bg->getheight(), SRCCOPY);

                // ��ʾ��ǰ��߷�
                setbkmode(TRANSPARENT);
                settextstyle(50, 0, _T("΢���ź�"));
                settextcolor(BLACK);
                TCHAR highScoreText[64];
                _stprintf_s(highScoreText, _T("��ǰ��߷֣�%d"), state.tempHighScore);
                outtextxy(20, 20, highScoreText);  // ���Ͻ���ʾ

                // ��ʾ�³ɾ���ʾ
                if (state.gotNewAchievement) {
                    setbkmode(TRANSPARENT);
                    settextstyle(50, 0, _T("΢���ź�"));
                    settextcolor(RED);
                    outtextxy(WIN_WIDTH - 300, 20, _T("�������³ɾͣ�"));
                }

                // ��ʾ��Ϸ����
                setbkmode(TRANSPARENT);
                settextstyle(108, 0, _T("΢���ź�"));
                settextcolor(RGB(255, 105, 180));  // ��ɫ����
                SIZE sz;
                GetTextExtentPoint32(GetImageHDC(), _T("���ɽ���ջ�"), _tcslen(_T("���ɽ���ջ�")), &sz);
                outtextxy((WIN_WIDTH - sz.cx) - 375, 60, _T(" �� �� ɽ �� �� �� "));  // ������ʾ

                // ����������ť����ʼ��Ϸ���������ɾͣ�
                settextstyle(40, 0, _T("΢���ź�"));
                setfillcolor(RGB(100, 200, 100));  // ��ť��ɫ��ǳ��ɫ��
                fillrectangle(btn_start.left, btn_start.top, btn_start.right, btn_start.bottom);  // ��ʼ��Ϸ��ť
                fillrectangle(btn_help.left, btn_help.top, btn_help.right, btn_help.bottom);      // ������ť
                fillrectangle(btn_achieve.left, btn_achieve.top, btn_achieve.right, btn_achieve.bottom);  // �ɾͰ�ť

                // ��ť����
                settextcolor(BLACK);
                outtextxy(btn_start.left + 30, btn_start.top + 10, _T("       ��ʼ��Ϸ"));
                outtextxy(btn_help.left + 30, btn_help.top + 10, _T("       ��Ϸ˵��"));
                outtextxy(btn_achieve.left + 30, btn_achieve.top + 10, _T("       �ɾ�ϵͳ"));

                FlushBatchDraw();  // ˢ�»���

                // �������˵�����¼�
                while (peekmessage(&msg)) {
                    if (msg.message == WM_LBUTTONDOWN) {  // ������
                        int x = msg.x, y = msg.y;
                        if (PtInRect(&btn_start, { x,y })) return 1;  // �����ʼ��Ϸ������1
                        if (PtInRect(&btn_help, { x,y })) {  // �������
                            mciSendString(_T("stop menu"), NULL, 0, NULL);  // ֹͣ�˵�����
                            state.isInInstruction = true;  // ��ǽ���˵������
                            return 0;
                        }
                        if (PtInRect(&btn_achieve, { x,y })) {  // ����ɾ�
                            state.gotNewAchievement = 0;  // ����ɾͽ�����³ɾ���ʾ��ʧ
                            achieveMenu.Run();  // ���гɾͲ˵�
                            return 0;
                        }
                    }
                }
            }
        }
    }
};

/**
 * ��ը������
 * ���ܣ�����ըЧ����֡����������֡ͼƬ�����Ŷ�����
 */
class ExplodeAnimation {
public:
    POINT pos;          // ��ըλ��
    int frame = 0;      // ��ǰ֡
    bool active = false;// �����Ƿ񼤻�
    static const int maxFrame = 24;  // ��֡��
    static unique_ptr<IMAGE> frames[maxFrame];

    /**
     * ���ر�ը������Դ
     * ���ܣ����ļ��������б�ը֡ͼƬ
     */
    static void LoadResources() {
        for (int i = 0; i < maxFrame; i++) {
            TCHAR path[64];
            _stprintf_s(path, _T("image\\explode%d.png"), i);  // ͼƬ·����explode0.png��explode23.png��
            frames[i] = std::make_unique<IMAGE>();  // �Զ������ڴ�
            loadimage(frames[i].get(), path);
        }
    }

    /**
     * ��ʼ��ը����
     * @param p ��ը����λ��
     * ���ܣ���ʼ������״̬��׼������
     */
    void Start(POINT p) {
        pos = p;
        frame = 0;
        active = true;
    }

    /**
     * ���Ʊ�ը����
     * ���ܣ����ݵ�ǰ֡���Ʊ�ըͼƬ���Զ��ƽ�֡����������Ϊ�Ǽ���
     */
    void Draw() {
        if (!active) return;  // δ�����򲻻���
        if (frame < maxFrame) {  // δ����������֡
            // Modify the line causing the error to explicitly get the raw pointer from the unique_ptr  
            putimage_alpha(pos.x - frames[frame]->getwidth() / 2, pos.y - frames[frame]->getheight() / 2, frames[frame].get());
            frame++;  // �ƽ�����һ֡
        }
        else active = false;  // ����������֡�����Ϊ�Ǽ���
    }

    bool IsActive() { return active; }  // ���ض����Ƿ񼤻�
};
unique_ptr<IMAGE> ExplodeAnimation::frames[ExplodeAnimation::maxFrame];

/**
 * ��ʯ��
 * ���ܣ���ʾ��Ϸ�еĿ�ʯ��������ʯͷ����ʯ�ȣ������������Ժͻ���
 */
class Mine {
public:
    POINT pos;          // λ��
    int type;           // ���ͣ�0=��1=ʯͷ��2=��ʯ�ȣ�
    int value;          // ��ֵ��������
    int damage;         // �����ƻ��ȣ��ɼ�ʱ���ӣ�
    bool alive = true;  // �Ƿ���ڣ�δ���ɼ���ը��
    IMAGE* img;         // ��ʯͼƬ
    ExplodeAnimation explodeAnim;  // ��ը����
    bool isExploding = false;  // �Ƿ����ڱ�ը�������ظ�������
    int pullSpeed;      // �������������ٶȣ�ֵԽСԽ����

    // ���캯������ʼ����ʯ����
    Mine(int x, int y, int t, int v, int d, IMAGE* i, int speed)
        :pos({ x,y }), type(t), value(v), damage(d), img(i), pullSpeed(speed) {
    }

    /**
     * ���ƿ�ʯ
     * ���ܣ����ƿ�ʯͼƬ�������ڱ�ը����Ʊ�ը����
     */
    void Draw() {
        if (alive) {  // ��ʯ����ʱ����ͼƬ
            putimage_alpha(pos.x, pos.y, img);
        }
        if (isExploding) {  // ���ڱ�ըʱ���ƶ���
            explodeAnim.Draw();
        }
    }
};

/**
 * ����ը��Χ
 * @param center ��ը����
 * @param mines ��ʯ�б�
 * @param envDamage �����ƻ��ȣ����ã������ۼӣ�
 * ���ܣ���ⱬը��Χ�ڵĿ�ʯ��ʹ����ʧ�������ƻ��ȣ�������ʽ��ը��ը����������ը����
 */
void HandleExplosion(POINT center, vector<Mine*>& mines, int& envDamage) {
    const int EXPLODE_RADIUS = 150;  // ��ը�뾶
    for (auto& mine : mines) {
        if (!mine->alive || mine->isExploding) continue;  // ��������ʧ�����ڱ�ը�Ŀ�ʯ

        // �����ʯ�����뱬ը���ĵľ���
        int dx = mine->pos.x + mine->img->getwidth() / 2 - center.x;
        int dy = mine->pos.y + mine->img->getheight() / 2 - center.y;
        // ��ƽ�������жϣ����⿪�������Ч�ʣ�
        const int EXPLODE_RADIUS_SQ = EXPLODE_RADIUS * EXPLODE_RADIUS;
        if (dx * dx + dy * dy < EXPLODE_RADIUS_SQ) {
            if (mine->type == 4) {  // ����ʯ��ը��������4����������ʽ��ը
                mine->isExploding = true;  // ���Ϊ���ڱ�ը
                mine->alive = false;       // ��ʯ��ʧ
                // �Ե�ǰը��Ϊ����������ը����
                POINT subCenter = {
                    mine->pos.x + mine->img->getwidth() / 2,
                    mine->pos.y + mine->img->getheight() / 2
                };
                mine->explodeAnim.Start(subCenter);
                envDamage += 10;  // ը����ը����10�ƻ���
                HandleExplosion(subCenter, mines, envDamage);  // �ݹ鴦����ʽ��ը
            }
            else {  // ��ը����ʯ��ֱ����ʧ�������ƻ���
                mine->alive = false;
                envDamage += mine->damage;
            }
        }
    }
}

/**
 * ������
 * ���ܣ������ӵ�״̬���ڶ������䡢�ջأ������ʯ�Ľ�������ȡ���ͷš�������
 */
class Hook {
private:
    POINT base{ WIN_WIDTH / 2,80 };  // ���ӻ���λ�ã������м䣩
    double angle = 0;                // �ڶ��Ƕȣ��ȣ�
    int dir = 1;                     // �ڶ�����1=�ң�-1=��
    int len = 50;                    // ���ӳ���
    int hookState = 0;                   // ״̬��0=�ڶ���1=�����У�2=�ջ��У�
    int speed = 8;                   // ����/�ջ��ٶ�
    IMAGE* normalImg;                // ��ͨ����ͼƬ
    IMAGE* catchImg;                 // ��ס��ʯʱ�Ĺ���ͼƬ
    Mine* caught = nullptr;          // ��ס�Ŀ�ʯ�� nullptr��ʾδ��ס��
    int currentPullSpeed;            // ��ǰ�ջ��ٶȣ��ܿ�ʯ����Ӱ�죩
    bool isCatching = false;         // �Ƿ����ڹ�ס��ʯ
    ExplodeAnimation explodeAnim;    // ��ը����
    vector<Mine*>& mines;            // ���õ�ǰ�ؿ��Ŀ�ʯ�б����ڱ�ը��⣩
    GameState& state;  // ������Ϸ״̬
public:
    // ���캯������ʼ������ͼƬ�Ϳ�ʯ�б�����
    Hook(IMAGE* normal, IMAGE* catchImg, vector<Mine*>& minesRef)
        : state(GameState::Instance()), normalImg(normal), catchImg(catchImg), mines(minesRef) {
    }

    /**
     * �ͷŹ�ס�Ŀ�ʯ
     * ���ܣ�����ס�Ŀ�ʯ�Żس��������ù���״̬
     */
    void Release() {
        if (hookState == 2 && caught != nullptr) {  // �����ջ��ҹ�ס�˿�ʯ
            caught->alive = true;  // ��ʯ���³����ڳ�����
            caught = nullptr;      // ���Ӳ��ٹ�ס��ʯ
            currentPullSpeed = 8;  // �����ջ��ٶ�
            isCatching = false;    // ���δ��ס��ʯ
        }
    }

    /**
     * ����ը��
     * ���ܣ�����ס����ը����������ը������ը��Χ
     */
    void Explode() {
        // ��ס��ը����δ���ڱ�ը
        if (caught && caught->type == 4 && !caught->isExploding) {
            mciSendString(_T("play audio\\explode.mp3 from 0"), NULL, 0, NULL);  // ���ű�ը��Ч
            state.envDamage += 10;  // ���ӻ����ƻ���
            POINT e = GetEnd();  // ��ȡ����ĩ��λ�ã���ը���ģ�
            explodeAnim.Start(e);  // ������ը����

            // ���ը��Ϊ���ڱ�ը
            caught->isExploding = true;
            caught->alive = false;
            HandleExplosion(e, mines, state.envDamage);  // ����ը��Χ�ڵĿ�ʯ

            // ���ù���״̬
            caught = nullptr;
            currentPullSpeed = 8;
            isCatching = false;
            hookState = 2;  // �����ջ�
        }
    }

    /**
     * ���¹���״̬
     * ���ܣ����ݵ�ǰ״̬���ڶ������䡢�ջأ�����λ�úͽǶ�
     */
    void Update() {
        if (hookState == 0) {  // �ڶ�״̬
            angle += dir;  // �ı�Ƕȣ��ڶ���
            // �������Ƕ�ʱ����
            if (angle > MAX_ANGLE) dir = -1;
            if (angle < -MAX_ANGLE) dir = 1;
        }
        else if (hookState == 1) {  // ������
            len += speed;  // ���ӹ��ӳ��ȣ��������죩
            // ���ŷ�����Ч����һ�Σ�
            if (!state.isLaunchBGMPlaying) {
                mciSendString(_T("play audio\\launch.mp3 from 0"), NULL, 0, NULL);
                state.isLaunchBGMPlaying = 1;
            }
            POINT e = GetEnd();  // ��ȡ����ĩ��λ��
            // ���ӳ������ڷ�Χ��ʼ�ջ�
            if (e.y > WIN_HEIGHT || e.x < 0 || e.x > WIN_WIDTH) {
                hookState = 2;
                currentPullSpeed = 8;
            }
        }
        else if (hookState == 2) {  // �ջ���
            len -= currentPullSpeed;  // ���ٹ��ӳ��ȣ������ջأ�
            state.isLaunchBGMPlaying = 0;   // ֹͣ������Ч���

            if (caught) {  // ��ס��ʯʱ�����¿�ʯλ�ã��湳���ƶ���
                POINT hookPos = GetEnd();
                caught->pos.x = hookPos.x - caught->img->getwidth() / 2;  // ��ʯ�����빳�Ӷ���
                caught->pos.y = hookPos.y - 10;
                // �����ʯ��λ��΢��
                if (caught->type == 0) caught->pos.y += 10;  // ���ʯ
                if (caught->type == 2) caught->pos.y += 5;   // ��ʯ
            }

            // �����ջ�����ʼ����
            if (len <= 50) {
                len = 50;  // ���ó���
                if (caught) {  // ��ס��ʯ�����ջ�������
                    state.totalScore += caught->value;  // ���ӷ���
                    state.envDamage += caught->damage;  // ���ӻ����ƻ���

                    // ������Ӧ�ɾ�
                    if (!state.achieve_first) state.achieve_first = true;  // �״ο���
                    if (caught->type == 3 && !state.achieve_screw) state.achieve_screw = true;  // ʳ����˿
                    if (caught->type == 4 && !state.achieve_bomb) state.achieve_bomb = true;    // ըҩ

                    mciSendString(_T("play audio\\score.mp3 from 0"), NULL, 0, NULL);  // ���Ż�ȡ��Ч
                    caught->alive = false;  // ��ʯ��ʧ
                    caught = nullptr;       // ���Ӳ��ٹ�ס��ʯ
                    isCatching = false;     // ���δ��ס
                }
                hookState = 0;  // �ص��ڶ�״̬
            }
        }
    }

    /**
     * ��ȡ����ĩ��λ��
     * ���ܣ����ݵ�ǰ�ǶȺͳ��ȼ��㹳��ĩ������
     */
    POINT GetEnd() {
        double r = angle * 3.1415926 / 180.0;  // �Ƕ�ת����
        // �������Ǻ�������ĩ��λ�ã�����Ϊ��㣩
        return {
            base.x + (int)(sin(r) * len),
            base.y + (int)(cos(r) * len)
        };
    }

    /**
     * ���ƹ���
     * ���ܣ����ƹ����ߺ͹���ͼƬ������ס��ʯ����ƿ�ʯ
     */
    void Draw() {
        POINT e = GetEnd();  // ��ȡĩ��λ��
        if (caught) {  // ��ס��ʯʱ���ƿ�ʯ
            caught->Draw();
        }
        // ���ƹ����ߣ��ӻ�����ĩ�ˣ�
        setlinecolor(BLACK);
        setlinestyle(PS_SOLID, 5);
        line(base.x, base.y, e.x, e.y);
        // ѡ����ͼƬ����ס/δ��ס��
        IMAGE* currentImg = isCatching ? catchImg : normalImg;
        // ���㹳��ͼƬƫ�ƣ�ʹͼƬ������ĩ�˶��룩
        int offsetX = -currentImg->getwidth() / 2;
        int offsetY = -currentImg->getheight() / 2 + 7;
        if (isCatching) offsetY += 5;  // ��סʱ΢��λ��
        // ���ƹ���ͼƬ
        putimage_alpha(e.x + offsetX, e.y + offsetY, currentImg);
        // ���Ʊ�ը���������У�
        explodeAnim.Draw();
    }

    /**
     * ���乳��
     * ���ܣ��Ӱڶ�״̬�л�Ϊ����״̬
     */
    void Launch() { if (hookState == 0) hookState = 1; }

    /**
     * ����Ƿ�ס��ʯ
     * @param m ��ʯ����
     * ���ܣ��жϹ���ĩ���Ƿ����ʯ��ײ��������ס��ʯ
     */
    bool Hooking(Mine* m) {
        if (caught) return false;  // �ѹ�ס��ʯ�򷵻�
        POINT e = GetEnd();  // ����ĩ��λ��
        // ��ʯ��ߵ�һ�루������ײ��⣩
        int mw = m->img->getwidth(), mh = m->img->getheight();
        // ��⹳��ĩ���Ƿ��ڿ�ʯ��Χ�ڣ�����ײ��⣩
        if (m->alive && abs(e.x - (m->pos.x + mw / 2)) < mw / 2 && abs(e.y - (m->pos.y + mh / 2)) < mh / 2) {
            caught = m;                // ��ס��ʯ
            currentPullSpeed = m->pullSpeed;  // �趨�ջ��ٶȣ���ʯԽ���ٶ�Խ����
            mciSendString(_T("play audio\\hit.mp3 from 0"), NULL, 0, NULL);  // ������ײ��Ч
            hookState = 2;                 // �л�Ϊ�ջ�״̬
            isCatching = true;         // ������ڹ�ס
            return true;
        }
        return false;
    }

    /**
     * �жϹ����Ƿ������ջ�
     * ���ܣ����ڿ��ƿ�ʯ����ʱ�����ջ�ʱ�������¿�ʯ��
     */
    bool Retracting() { return hookState == 2; }
};

/**
 * �ɿ���Ϸ��
 * ���ܣ��������ؿ�����Ϸ�߼�����ʯ���ɡ����¡����ơ����봦��
 */
class MinerGame {
private:
    vector<Mine*> mines;  // ��ʯ�б�
    Hook* hook;           // ���Ӷ���
    IMAGE* bg;            // �ؿ�����ͼ
    ULONGLONG lastTimeCheck;  // ��һ�μ��ʱ�䣨���ڵ���ʱ��
    GameState& state;  // ������Ϸ״̬
    int level;            // ��ǰ�ؿ���
    bool levelOver = false;  // �����ؿ��Ƿ����

public:
    /**
     * ���캯��
     * @param b ����ͼ
     * @param normalHook ��ͨ����ͼƬ
     * @param catchHook ��סʱ�Ĺ���ͼƬ
     * @param imgs ��ʯͼƬ�б�
     * @param level �ؿ���
     * ���ܣ���ʼ���ؿ������ɿ�ʯ�����ӣ�
     */
    MinerGame(IMAGE* b, IMAGE* normalHook, IMAGE* catchHook, vector<IMAGE*> imgs, int level) : bg(b), state(GameState::Instance()), level(level) {
        hook = new Hook(normalHook, catchHook, mines);  // �������ӣ������ʯ�б����ã�
        srand((unsigned)time(nullptr));  // ���������
        lastTimeCheck = GetTickCount64();  // ��ʼ������ʱ���ʱ��
        // ���õ�ǰ�ؿ���ʱ��
        state.gameTime = state.levelTimes[level - 1];
        // ���õ�ǰ�ؿ��Ļ�������
        state.baseScoreReduction = level * 4;
        // ���ؿ���ʯ���������ݹؿ��Ѷȵ�����
        int countGold = 0, countStone = 0, countDiamond = 0, countScrew = 0, countBomb = 0, countCoal = 0, countIron = 0;
        if (level == 1) {  // ��һ�أ�������ʯ
            countStone = 4; countBomb = 2; countScrew = 6; countGold = 0; countCoal = 3; countIron = 0;
        }
        else if (level == 2) {  // �ڶ��أ����Ӽ�ֵ�ϸߵĿ�ʯ
            countStone = 4; countGold = 2; countScrew = 4; countDiamond = 0; countBomb = 4; countCoal = 4; countIron = 2;
        }
        else if (level == 3) {  // �����أ�������ʯ������
            countStone = 6; countGold = 3; countDiamond = 2; countBomb = 8; countCoal = 2; countIron = 4; countScrew = 0;
        }
        else if (level == 4) {  // ���Ĺأ��߼�ֵ��ʯΪ��
            countStone = 6; countGold = 4; countDiamond = 3; countBomb = 8; countCoal = 0; countIron = 2; countScrew = 0;
        }
        else if (level == 5) {  // ����أ�����Ѷ�
            countStone = 8; countGold = 6; countDiamond = 4; countBomb = 8; countCoal = 0; countIron = 0; countScrew = 0;
        }

        // ���ӻ���λ�ã����ڿ�ʯ���ɵİ�ȫ������㣩
        POINT hookBase = { WIN_WIDTH / 2, 80 };
        // ��ȫ���룺�빳������150���أ���ʯ֮������64����
        const int SAFE_DISTANCE_TO_HOOK = 150;
        const int SAFE_DISTANCE_BETWEEN_MINES = 64;

        // ��ʯ���ɺ�����lambda���ʽ�����ظ����룩
        auto spawn = [&](int type, int count) {
            for (int i = 0; i < count; i++) {
                int x, y;
                bool ok;
                do {
                    ok = true;
                    // �������λ�ã����ڷ�Χ�ڣ�
                    x = rand() % (WIN_WIDTH - 64);
                    y = rand() % (WIN_HEIGHT - 200) + 150;

                    // ����Ƿ��빳��̫��
                    if (sqrt(pow(x - hookBase.x, 2) + pow(y - hookBase.y, 2)) < SAFE_DISTANCE_TO_HOOK) {
                        ok = false;
                        continue;
                    }

                    // ����Ƿ���������ʯ̫��
                    for (auto m : mines) {
                        if (sqrt(pow(x - m->pos.x, 2) + pow(y - m->pos.y, 2)) < SAFE_DISTANCE_BETWEEN_MINES) {
                            ok = false;
                            break;
                        }
                    }
                } while (!ok);  // ֱ�����ɺϷ�λ��

                // ���ݿ�ʯ�����������ԣ���ֵ���ƻ��ȡ��ջ��ٶȣ�
                int v, d, speed;
                switch (type) {
                case 0: v = 500; d = 3; speed = 4; break;  // ��
                case 1: v = 50; d = 5; speed = 2; break;   // ʯͷ
                case 2: v = 1000; d = 2; speed = 8; break; // ��ʯ
                case 3: v = 1; d = 1; speed = 8; break;    // ��˿
                case 4: v = 0; d = 1; speed = 7; break;    // ը��
                case 5: v = 200; d = 3; speed = 4; break;  // ú��
                case 6: v = 300; d = 3; speed = 5; break;  // ����
                }
                // ��ӿ�ʯ���б�
                mines.push_back(new Mine(x, y, type, v, d, imgs[type], speed));
            }
            };

        // ���ɸ����Ϳ�ʯ
        spawn(0, countGold);
        spawn(1, countStone);
        spawn(2, countDiamond);
        spawn(3, countScrew);
        spawn(4, countBomb);
        spawn(5, countCoal);
        spawn(6, countIron);
    }

    /**
     * ��������
     * ���ܣ��ͷŹ��ӺͿ�ʯ���ڴ�
     */
    ~MinerGame() {
        delete hook;
        for (auto m : mines) delete m;  // �ͷ����п�ʯ
    }

    /**
     * ������Ϸ״̬
     * ���ܣ����µ���ʱ������״̬����ʯ��ײ���
     */
    void Update() {
        // ����ʱ�߼���ÿ�����1��
        ULONGLONG now = GetTickCount64();
        if (now - lastTimeCheck >= 1000 && !state.gameOver) {  // ���1��
            state.gameTime--;
            lastTimeCheck = now;
            if(state.totalScore>3000)
            state.totalScore = max(0, state.totalScore - state.baseScoreReduction);
            // ���10�벥����ʾ��Ч
            if (state.gameTime <= 10 && state.gameTime > 0 && !state.isTimeupBGMPlaying) {
                state.isTimeupBGMPlaying = 1;
                mciSendString(_T("play audio\\last-10s-sound.mp3 from 0"), NULL, 0, NULL);
            }
            // ʱ��������ؿ�����
            if (state.gameTime <= 0) {
                state.isTimeupBGMPlaying = 0;
                levelOver = true;
            }
        }

        if (!levelOver) {  // �ؿ�δ���������
            hook->Update();  // ���¹���
            if (hook->Retracting() || state.gameOver) return;  // �ջ�ʱ�������ײ
            // ��⹳���Ƿ�ס��ʯ
            for (auto m : mines) if (hook->Hooking(m)) break;
        }
    }

    /**
     * ������Ϸ����
     * ���ܣ����Ʊ�������ʯ�����ӡ�������ʱ�䡢�ƻ��ȣ�����ؿ������ͽ��
     */
    void Draw() {
        // ���Ʊ���
        StretchBlt(GetImageHDC(NULL), 0, 0, WIN_WIDTH, WIN_HEIGHT,
            GetImageHDC(bg), 0, 0, bg->getwidth(), bg->getheight(), SRCCOPY);

        // ���ƿ�ʯ
        for (auto m : mines) {
            if (m->alive) m->Draw();
        }
        // ���Ʊ�ը������ȷ���ڿ�ʯ�Ϸ���
        for (auto m : mines) {
            if (m->isExploding) m->explodeAnim.Draw();
        }

        // ���ƹ���
        hook->Draw();

        // ��ʾ������ʱ�䡢�����ƻ���
        TCHAR buf[64];
        settextcolor(BLACK);
        settextstyle(30, 0, _T("΢���ź�"));

        _stprintf_s(buf, _T("������%d"), state.totalScore);
        outtextxy(20, 20, buf);  // ���Ͻ�

        _stprintf_s(buf, _T("ʱ�䣺%d ��"), state.gameTime);
        outtextxy(20, 60, buf);

        _stprintf_s(buf, _T("�����ƻ��ȣ�%d"), state.envDamage);
        outtextxy(20, 100, buf);

        // �ؿ���������
        if (levelOver) {
            mciSendString(_T("stop mining"), NULL, 0, NULL);  // ֹͣ�ؿ�����
            ULONGLONG showStart = GetTickCount64();
            const int showDuration = 3000;  // ��ʾ3��ؿ�������ʾ

            mciSendString(_T("play audio\\timeup.mp3 from 0"), NULL, 0, NULL);  // ����ʱ�䵽��Ч

            // ��ʾ�ؿ�������ʾ
            while (GetTickCount64() - showStart < showDuration) {
                StretchBlt(GetImageHDC(NULL), 0, 0, WIN_WIDTH, WIN_HEIGHT,
                    GetImageHDC(bg), 0, 0, bg->getwidth(), bg->getheight(), SRCCOPY);
                for (auto m : mines) if (m->alive) m->Draw();
                hook->Draw();

                // ��ʾ��ʾ����
                setbkmode(TRANSPARENT);
                settextstyle(80, 0, _T("΢���ź�"));
                settextcolor(RED);
                const TCHAR* text = (level < state.MAX_LEVEL) ?
                    _T("Time is up!\n������������̽��......") :  // �����һ��
                    _T("Time is up!\n��Ϸ����!");  // ���һ��
                RECT r = { 0, WIN_HEIGHT / 3, WIN_WIDTH, WIN_HEIGHT };
                drawtext(text, &r, DT_CENTER | DT_VCENTER | DT_WORDBREAK);  // ������ʾ
                FlushBatchDraw();
                Sleep(16);
            }

            // ���һ�ؽ������ж����
            if (level == state.MAX_LEVEL) {
                state.gameOver = 1;  // �����Ϸ����
                mciSendString(_T("play end repeat from 0"), NULL, 0, NULL);  // ���Ž������

                // ���ݷ������ƻ����ж����
                const TCHAR* resultText;
                const TCHAR* desc;
                IMAGE* endingImg;

                if (state.totalScore < 5000 && state.envDamage < 100) {  // ���1�����ɲ���
                    state.achieve_underdev = true;
                    endingImg = &ending1;
                    resultText = _T("���1���ɼ�����");
                    desc = _T("��Сʵ����Դ�ɼ���δ�ﵽ����Ҫ���࿪��������ͺ�\n����δ���Ϲ��ڣ���������һƬ��ƺ��ѧУ�����ս�����...\n��ʾ���ڲ������ƻ����ɽ�����������ѡ��߼�ֵ��ʯ��!");
                }
                else if (state.totalScore >= 5000 && state.envDamage < 100) {  // ���2����������
                    state.achieve_perfectdev = true;
                    endingImg = &ending2;
                    resultText = _T("���2����������");
                    desc = _T("��Сʵ����ƽ���˿�������̬���࿪˳��������\nѧУ����ֱ���������оţ���ָC9��\n������࿪����ɽ��ʮ��������");
                }
                else {  // ���3����������
                    state.achieve_overdev = true;
                    endingImg = &ending3;
                    resultText = _T("���3����������");
                    desc = _T("��Сʵ�����ƻ���̬���࿪��˳������,�������Ѿ�������ѧ������,������Ϊ�Ҳ��̶õĻĵ�,ѧУ����ֱ���½���\n�����Ժ󣬵���Сʵ�ٴλص��࿪�����������꿪�����ɽ���Ǹ�����...");
                }

                // ���ƽ�ֻ���
                StretchBlt(GetImageHDC(NULL), 0, 0, WIN_WIDTH, WIN_HEIGHT,
                    GetImageHDC(endingImg), 0, 0, endingImg->getwidth(), endingImg->getheight(), SRCCOPY);

                // ��ʾ��ֱ���
                settextstyle(60, 0, _T("΢���ź�"));
                settextcolor(RED);
                SIZE sz;
                GetTextExtentPoint32(GetImageHDC(), resultText, _tcslen(resultText), &sz);
                outtextxy((WIN_WIDTH - sz.cx) / 2, WIN_HEIGHT / 4 - 50, resultText);

                // ��ʾ�������
                settextstyle(40, 0, _T("΢���ź�"), 0, 0, 700, false, false, false);
                settextcolor(RGB(255, 20, 147));
                RECT descRect = { 150, WIN_HEIGHT / 4 + 25, WIN_WIDTH - 100, WIN_HEIGHT / 2 + 100 };
                drawtext(desc, &descRect, DT_CENTER | DT_WORDBREAK);

                // ���Ʒ��ز˵���ť
                state.showReturnBtn = true;
                setfillcolor(RGB(100, 200, 100));
                fillrectangle(returnBtn.left, returnBtn.top, returnBtn.right, returnBtn.bottom);
                settextstyle(40, 0, _T("΢���ź�"));
                settextcolor(BLACK);
                outtextxy(returnBtn.left + 20, returnBtn.top + 10, _T("  ���ز˵�"));
                FlushBatchDraw();
            }
        }
    }

    /**
     * ���������¼�
     * @param msg ��Ϣ
     * ���ܣ���Ӧ���ͼ������루���乳�ӡ��ͷſ�ʯ������ը����
     */
    void Process(const ExMessage& msg) {
        if (msg.message == WM_LBUTTONDOWN && !state.gameOver)
            hook->Launch();  // ������乳��

        if (msg.message == WM_RBUTTONDOWN && !state.gameOver)
            hook->Release();  // �Ҽ��ͷſ�ʯ

        if (msg.message == WM_KEYDOWN && msg.vkcode == VK_SPACE && !state.gameOver)
            hook->Explode();  // �ո�����ը��
    }

    /**
     * �жϹؿ��Ƿ����
     */
    bool IsLevelOver() {
        return levelOver;
    }
};

/**
 * ͳ�Ƶ�ǰ�����ĳɾ�����
 * ���ܣ������ж��Ƿ����³ɾͣ�����Ϸǰ�������Աȣ�
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

// ȫ�ֱ�����ʼ��
vector<IMAGE*> imgs{ &goldImg, &rockImg, &diamondImg, &screwImg, &bombImg, &coalImg, &ironImg };
Menu menu(&menuBG);

/**
 * ������
 * ���ܣ�������ڣ���ʼ����Դ����ѭ�����˵�����Ϸ�л������ؿ����̿���
 */
int main() {
    initgraph(WIN_WIDTH, WIN_HEIGHT);  // ��ʼ��ͼ�δ���
    HWND hWnd = GetHWnd();
    SetWindowText(hWnd, _T("���ɽ���ջ�"));  // ���ô��ڱ���
    BeginBatchDraw();  // ��ʼ�������ƣ����Ч�ʣ�

    // �򿪱���������Դ��Ԥ���أ����ⲥ��ʱ���٣�
    mciSendString(_T("open audio\\main_menu_bgm.mp3 alias menu"), NULL, 0, NULL);
    mciSendString(_T("open audio\\cut_scene.mp3 alias cutscene"), NULL, 0, NULL);
    mciSendString(_T("open audio\\instruction_bgm.mp3 alias instruction"), NULL, 0, NULL);
    mciSendString(_T("open audio\\ending_bgm.mp3 alias end"), NULL, 0, NULL);
    mciSendString(_T("open audio\\mining_bgm.mp3 alias mining"), NULL, 0, NULL);

    // ����ͼƬ��Դ
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

    ExplodeAnimation::LoadResources();  // ���ر�ը����֡

    setlinecolor(BLACK);
    setlinestyle(PS_SOLID, 5);

    // ��ѭ��
    while (GameState::Instance().isRunning) {
        ExMessage msg;
        while (peekmessage(&msg)) {
            if (msg.message == WM_CLOSE) {  // �û�����رհ�ť
                GameState::Instance().isRunning = false;  // ����˳�ѭ��
                break;
            }
        }
        if (!GameState::Instance().isRunning) break;  // �յ��ر��¼����˳�ѭ��
        if (GameState::Instance().isInMenu) {  // �����˵�
            int menuResult = menu.Run();
            if (menuResult == 1) {  // �����ʼ��Ϸ
                GameState& state = GameState::Instance();
                state.lastAchieveCount = GetCurrentAchieveCount();  // ��¼��ǰ�ɾ���

                // ��ʼ����Ϸ״̬
                state.isInMenu = false;
                state.totalScore = 0;
                state.envDamage = 0;
                state.currentLevel = 1;
                state.gameOver = false;
                state.showReturnBtn = false;
                ExMessage msg;

                mciSendString(_T("stop menu"), NULL, 0, NULL);  // ֹͣ�˵�����

                // �ؿ����̣���1��MAX_LEVEL��
                while (state.currentLevel <= state.MAX_LEVEL) {
                    ShowCutScene(state.currentLevel);  // ���Ź�������
                    mciSendString(_T("play mining repeat from 0"), NULL, 0, NULL);  // ���Źؿ�����

                    // ���õ�ǰ�ؿ�ʱ�䣨��ؿ����ӣ�
                    state.gameTime = 80 - (state.currentLevel - 1) * 10;
                    MinerGame game(&mineBG, &hookImg, &catchImg, imgs, state.currentLevel);  // �����ؿ�

                    // �����ؿ�����Ϸѭ��
                    while (!game.IsLevelOver()) {
                        ULONGLONG t0 = GetTickCount64();  // ��¼��ǰʱ��

                        // ��������
                        while (peekmessage(&msg)) {
                            game.Process(msg);
                        }

                        game.Update();  // ������Ϸ״̬
                        game.Draw();    // ������Ϸ����
                        FlushBatchDraw();  // ˢ��

                        // ����֡�ʣ�Լ60֡��
                        ULONGLONG t1 = GetTickCount64();
                        if (t1 - t0 < 1000 / GAME_FRAME)
                            Sleep(1000 / GAME_FRAME - (t1 - t0));
                    }
                    state.currentLevel++;  // ������һ��
                }

                // ��Ϸ�����󣬵ȴ����ز˵�
                while (!state.isInMenu) {
                    while (peekmessage(&msg)) {
                        if (msg.message == WM_LBUTTONDOWN) {  // ������ذ�ť
                            int x = msg.x, y = msg.y;
                            if (PtInRect(&returnBtn, { x, y })) {
                                state.isInMenu = true;  // ���ز˵�
                                break;
                            }
                        }
                    }
                }

                // ����Ƿ����³ɾ�
                state.gotNewAchievement = GetCurrentAchieveCount() > state.lastAchieveCount;
                // ������߷�
                if (state.totalScore > state.tempHighScore) {
                    state.tempHighScore = state.totalScore;
                }

                // ����״̬
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