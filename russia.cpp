#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <assert.h>
#include <limits.h>
#ifdef _DEBUG
#define verify(f)    assert(f)
#else
#define verify(f)    ((void)(f))
#endif
class Console // 控制台
{
    friend class Window;
public:
    // title: 控制台标题
    // point: 控制台宽度和高度
    void Open( const char* title, COORD wl )
    {
        assert( wl.X>0 && wl.Y>0 );
        verify( (hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE)) != INVALID_HANDLE_VALUE ); // 获得输出句柄
        verify( SetConsoleTitle( title ) );                        // 设置标题
        CONSOLE_CURSOR_INFO cci = { 25, FALSE };
        verify( SetConsoleCursorInfo( hStdOutput, &cci ) );        // 去处光标
        SMALL_RECT wrt = { 0, 0, wl.X-1, wl.Y-1 };
        verify( SetConsoleWindowInfo( hStdOutput, TRUE, &wrt ) );  // 设置窗体尺寸
        coord = wl;                                      
        verify( SetConsoleScreenBufferSize( hStdOutput, coord ) ); // 设置缓冲尺寸
    }
private:
    HANDLE hStdOutput;
    COORD  coord;
};
class Window // 窗体是控制台的一个子部分
{
public:
    // wrect: 窗体左上角坐标,右下角坐标
    void Open( Console& console, SMALL_RECT wrect )
    {
        assert( wrect.Left < wrect.Right && wrect.Top < wrect.Bottom );
        assert( wrect.Left>=0 && wrect.Right<=console.coord.X );
        assert( wrect.Top>=0 && wrect.Bottom<=console.coord.Y );
        pcon = &console;
        wrt = wrect;
    }
    // x,y: 相对于窗体的坐标
    // color: 字体和背景颜色
    // str: 欲输出字符串,遇0或达到len数量则结束,没考虑控制字符,比如\r等
    void Output( short x, short y, WORD color, const char* str, size_t len=INT_MAX )
    {
        assert( x>=0 && x<=wrt.Right-wrt.Left && y>=0 && y<=wrt.Bottom-wrt.Top );
        COORD c = { wrt.Left+x, wrt.Top+y };
        DWORD n = 0;
        WORD cs[2] = { color, color };
        for( const char* p=str; len!=0 && *p!=0; --len,++p,++c.X  )
        {
            if( c.X >= wrt.Right ) // 换行
            {
                c.X = wrt.Left+x;
                ++c.Y;
                assert( c.Y < wrt.Bottom );
            }
            if( *p > 0 ) // 单字节字符
            {
                verify( WriteConsoleOutputCharacter( pcon->hStdOutput, p, 1, c, &n ) && n==1 );
                verify( WriteConsoleOutputAttribute( pcon->hStdOutput, cs, 1, c, &n ) && n==1 );
            }
            else // 双字节字符
            {
                assert( len>=2 && *(p+1)!=0 && (c.X+1)<wrt.Right );
                verify( WriteConsoleOutputCharacter( pcon->hStdOutput, p, 2, c, &n ) && n==2 );
                verify( WriteConsoleOutputAttribute( pcon->hStdOutput, cs, 2, c, &n ) && n==2 );
                --len,++p,++c.X;
            }
        }
    }
private:
    Console*  pcon;
    SMALL_RECT wrt;
};
class RussiaDiamonds
{
public:
    // x,y: 左上角坐标,右下角坐标
    RussiaDiamonds( Console& console, short x, short y )
    {
        SMALL_RECT wrect = { x, y, x+38, y+21 };
        win.Open( console, wrect );
    }
    // 分别是 开始,暂停,声效,变形,左移,右移,下移,落地 键的键码, 声效频率, 延续时间
    // Example:
    //  A   键为 0x41,      用0x00000041 表示
    //  F1  键为 0x00,0x3B, 用0x00003800 表示
    //  F11 键为 0xE0,0x85, 用0x000085E0 表示
    void Open( int ctrlkeys[8], char ctrlkeydecs[8][5], DWORD frequency, DWORD duration )
    {
        memcpy( keys, ctrlkeys, sizeof(keys) );
        memcpy( decs, ctrlkeydecs, sizeof(decs) );
        freq = frequency, dura =duration;
        gameover = false;
        pause = true;
        voice = true;
        score = 0, level = 0;
        memset( data, 0, sizeof(data) );
        srand( time(0) );
        next = rand()%7;
        x=4, y=-2, c=-1, z=0;
        win.Output( 0, 0, COLOR_B, bg+0, 38 );
        for( int i=1; i<20; ++i )
        {
            win.Output( 0, i, COLOR_B, bg+38*i+0, 2 );
            win.Output( 2, i, COLOR_C, bg+38*i+2, 22 );
            win.Output( 24, i, COLOR_B, bg+38*i+24, 14 );
        }
        win.Output( 0, 20, COLOR_B, bg+38*20, 38 );
        for( int j=0; j<8; ++j )
            win.Output( 33, j+7, COLOR_B, decs[j], 4 );
        DrawNext();
    }
    bool IsRun() const
    {
        return !gameover && !pause;
    }
    int Level() const
    {
        return level;
    }
    bool Down( void )
    {
        return MessageDeal( keys[6] );
    }
    // 游戏结束返回 false
    bool MessageDeal( const int k )
    {
        if( gameover )
        {
            if( k == keys[0] ) // 重新开始
            {
                Open( keys, decs,freq, dura );
                return true;
            }
            return false;
        }
        if( pause ) // 暂停
        {
            if( k == keys[0] )      // 开始
            {
                pause = false;
                if( c == -1 )
                {
                    c = next;
                    next = rand()%7;
                    DrawNext();
                }
            }
            else if( k == keys[2] ) // 声效
                voice = !voice;
            else
                return true;
            VoiceBeep();
            return true;
        }
        if( k == keys[1] )      // 暂停
            pause = true;
        else if( k == keys[3] ) // 变形
            MoveTrans();
        else if( k == keys[4] ) // 左移
            MoveLeft();
        else if( k == keys[5] ) // 右移
            MoveRight();
        else if( k == keys[6] ) // 下移
            { if( 0 == MoveDown() ) return false; }
        else if( k == keys[7] ) // 落地
            { if( !MoveDownDown() ) return false; }
        else if( k == keys[2] ) // 声效
            voice = !voice;
        else
            return true;
        VoiceBeep();
        return true;
    }
private:
    void VoiceBeep( void ) // 声效
    {
        if( voice ) Beep( freq, dura );
    }
    void DrawScoreLevel( void ) // 绘制得分
    {
        char tmp[6];
        sprintf( tmp, "%05d", score );
        win.Output( 31, 19, COLOR_B, tmp, 5 );
        sprintf( tmp, "%1d", level );
        win.Output( 28, 19, COLOR_B, tmp, 1 );
    }
    void DrawNext( void )  // 绘制 "next框" 中的图形
    {
        for( int i=0; i<2; ++i )
            for( int j=0; j<4; ++j )
                win.Output( 28+j*2, 1+i, COLOR_B, bk[next][0][i][j]==0?"　":"■", 2 );
    }
    void DrawOver( void )  // 游戏结束
    {
        win.Output( 28, 1, COLOR_B, "ＧＡＭＥ" );
        win.Output( 28, 2, COLOR_B, "ＯＶＥＲ" );
    }
    void Draw( WORD color )
    {
        for( int i=0; i<4; ++i )
        {
            if( y+i<0 || y+i>= 19 ) continue;
            for( int j=0; j<4; ++j )
                if( bk[c][z][i][j] == 1 )
                    win.Output( 2+x*2+j*2, 1+y+i, color, "■", 2 );
        }
    }
    bool IsFit( int x, int y, int c, int z ) // 给定的x,y,c,z是否可行
    {
        for( int i=0; i<4; ++i )
        {
            for( int j=0; j<4; ++j )
            {
                if( bk[c][z][i][j]==1 )
                {
                    if( y+i < 0 ) continue;
                    if( y+i>=19 || x+j<0 || x+j>=11 || data[y+i][x+j]==1 ) return false;
                }
            }
        }
        return true;
    }
    void RemoveRow( void ) // 消行
    {
        const char FULLLINE[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        int linecount = 0;
        for( int i=0; i<19; ++i )
        {
            if( 0 == memcmp( data[i], FULLLINE, 11 ) )
            {
                ++linecount;
                for( int m=0; m<11; ++m )
                {
                    for( int n=i; n>1; --n )
                    {
                        data[n][m] = data[n-1][m];
                        win.Output( 2+m*2, 1+n, data[n][m]==1?COLOR_B:COLOR_C, "■", 2 );
                    }
                    data[0][m] = 0;
                    win.Output( 2+m*2, 1, COLOR_C, "■", 2 );
                }
            }
        }
        char data[19][11] = { 0 };
        if( linecount == 0 ) return;
        int _score = 0;
        switch( linecount )
        {
        case 1: _score =  100; break;
        case 2: _score =  300; break;
        case 3: _score =  700; break;
        case 4: _score = 1500; break;
        }
        score += _score;
        if( score > 99999 ) score = 99999;
        level = score/10000;
        DrawScoreLevel();
    }
    void MoveTrans( void )  // 逆时针翻转
    {
        if( IsFit( x, y, c, (z+1)%4 ) )
        {
            VoiceBeep();
            Draw( COLOR_C );
            z=(z+1)%4;
            Draw( COLOR_A );
        }
    }
    void MoveLeft( void )    // 向左移
    {
        if( IsFit( x-1, y, c, z ) )
        {
            VoiceBeep();
            Draw( COLOR_C );
            --x;
            Draw( COLOR_A );
        }
    }
    void MoveRight( void )  // 向右移
    {
        if( IsFit( x+1, y, c, z ) )
        {
            VoiceBeep();
            Draw( COLOR_C );
            ++x;
            Draw( COLOR_A );
        }
    }
    int MoveDown( void )   // 向下移, 游戏结束返回0, 触底返回-1,否则返回1
    {
        if( IsFit( x, y+1, c, z ) )
        {
            VoiceBeep();
            Draw( COLOR_C );
            ++y;
            Draw( COLOR_A );
            return 1;
        }
        if( y != -2 ) // 触底
        {
            Draw( COLOR_B );
            for( int i=0; i<4; ++i )
            {
                if( y+i<0 ) continue;
                for( int j=0; j<4; ++j )
                    if( bk[c][z][i][j] == 1 )
                        data[y+i][x+j] = 1;
            }
            RemoveRow();
            x=4, y=-2, c=next, z=0;
            next = rand()%7;
            DrawNext();
            return -1;
        }
        // 游戏结束
        gameover = true;
        DrawOver();
        return 0;
    }
    bool MoveDownDown( void ) // 下落到底
    {
        int r;
        for( ; r=MoveDown(),r==1; );
        return r == -1;
    }
private:
    static const char bg[21*38+1];
    static const char bk[7][4][4][4];
    static const WORD COLOR_A; // 运动中的颜色
    static const WORD COLOR_B; // 固定不动的颜色
    static const WORD COLOR_C; // 空白处的颜色
private:
    DWORD freq, dura; // 声效频率,延续时间
    int keys[8]; // 8个控制键
    char decs[8][5]; // 8个控制键的描述, 长度为4个字节, 不含\0
    bool gameover;
    bool pause; // 暂停
    bool voice; // 声效开关
    int score, level; // 得分 和 速度
    char data[19][11];
    int next;
    int x, y, c, z; // x坐标,y坐标,当前方块,方向
    Window win;
};
const char RussiaDiamonds::bg[21*38+1] = 
    "┏━━━━━━━━━━━┓┏━━━━┓"
    "┃■■■■■■■■■■■┃┃        ┃"
    "┃■■■■■■■■■■■┃┃        ┃"
    "┃■■■■■■■■■■■┃┗━━━━┛"
    "┃■■■■■■■■■■■┃            "
    "┃■■■■■■■■■■■┃ 退出= ESC  "
    "┃■■■■■■■■■■■┃            "
    "┃■■■■■■■■■■■┃ 开始=      "
    "┃■■■■■■■■■■■┃ 暂停=      "
    "┃■■■■■■■■■■■┃ 声效=      "
    "┃■■■■■■■■■■■┃ 变形=      "
    "┃■■■■■■■■■■■┃ 左移=      "
    "┃■■■■■■■■■■■┃ 右移=      "
    "┃■■■■■■■■■■■┃ 下移=      "
    "┃■■■■■■■■■■■┃ 落地=      "
    "┃■■■■■■■■■■■┃            "
    "┃■■■■■■■■■■■┃            "
    "┃■■■■■■■■■■■┃ 速度  得分 "
    "┃■■■■■■■■■■■┃┏━━━━┓"
    "┃■■■■■■■■■■■┃┃0  00000┃"
    "┗━━━━━━━━━━━┛┗━━━━┛";
const char RussiaDiamonds::bk[7][4][4][4] =
{
    {
        { { 0,1,1,0 },{ 1,1,0,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 1,0,0,0 },{ 1,1,0,0 },{ 0,1,0,0 },{ 0,0,0,0 } },
        { { 0,1,1,0 },{ 1,1,0,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 1,0,0,0 },{ 1,1,0,0 },{ 0,1,0,0 },{ 0,0,0,0 } }
    }
    ,
    {
        { { 1,1,0,0 },{ 0,1,1,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 0,1,0,0 },{ 1,1,0,0 },{ 1,0,0,0 },{ 0,0,0,0 } },
        { { 1,1,0,0 },{ 0,1,1,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 0,1,0,0 },{ 1,1,0,0 },{ 1,0,0,0 },{ 0,0,0,0 } }
    }
    ,
    {
        { { 1,1,1,0 },{ 1,0,0,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 1,0,0,0 },{ 1,0,0,0 },{ 1,1,0,0 },{ 0,0,0,0 } },
        { { 0,0,1,0 },{ 1,1,1,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 1,1,0,0 },{ 0,1,0,0 },{ 0,1,0,0 },{ 0,0,0,0 } }
    }
    ,
    {
        { { 1,1,1,0 },{ 0,0,1,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 1,1,0,0 },{ 1,0,0,0 },{ 1,0,0,0 },{ 0,0,0,0 } },
        { { 1,0,0,0 },{ 1,1,1,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 0,1,0,0 },{ 0,1,0,0 },{ 1,1,0,0 },{ 0,0,0,0 } }
    }
    ,
    {
        { { 1,1,0,0 },{ 1,1,0,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 1,1,0,0 },{ 1,1,0,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 1,1,0,0 },{ 1,1,0,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 1,1,0,0 },{ 1,1,0,0 },{ 0,0,0,0 },{ 0,0,0,0 } }
    }
    ,
    {
        { { 0,1,0,0 },{ 1,1,1,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 0,1,0,0 },{ 1,1,0,0 },{ 0,1,0,0 },{ 0,0,0,0 } },
        { { 1,1,1,0 },{ 0,1,0,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 1,0,0,0 },{ 1,1,0,0 },{ 1,0,0,0 },{ 0,0,0,0 } }
    }
    ,
    {
        { { 1,1,1,1 },{ 0,0,0,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 1,0,0,0 },{ 1,0,0,0 },{ 1,0,0,0 },{ 1,0,0,0 } },
        { { 1,1,1,1 },{ 0,0,0,0 },{ 0,0,0,0 },{ 0,0,0,0 } },
        { { 1,0,0,0 },{ 1,0,0,0 },{ 1,0,0,0 },{ 1,0,0,0 } }
    }
};
const WORD RussiaDiamonds::COLOR_A = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY;
const WORD RussiaDiamonds::COLOR_B = FOREGROUND_GREEN;
const WORD RussiaDiamonds::COLOR_C = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
void MessageDeal( RussiaDiamonds rds[], int num )
{
    int* counts = new int[num];
    memset( counts, 0, num*sizeof(int) );
    for( ; ; )
    {
        if( _kbhit() )
        {
            int k = _getch();
            if( k == 0x1B ) break; // 遇到 Esc 键则退出
            if( k==0 || k==0xE0 )  // 控制字符
                k = _getch()<<8 | k;
            else if( k>='a' && k<='z' ) // 小写字母转化为大写
                k = k - ( 'a'-'A' );
            for( int n=0; n<num; ++n )
                rds[n].MessageDeal( k );
        }
        for( int n=0; n<num; ++n )
        {
            if( rds[n].IsRun() && 10-rds[n].Level()==++counts[n] )
            {
                counts[n] = 0;
                rds[n].Down();
            }
        }
        Sleep( 55 );
    }
    delete[] counts;
}
int main()
{
//* 一个人玩
    Console csl;
    COORD crect = { 38, 21 };
    csl.Open( "俄罗斯方块", crect );
    int keys[8] = { 0x00000042, 0x00000053, 0x00000056, 0x000048E0, 0x00004BE0, 0x00004DE0, 0x000050E0, 0x00000020 };
    char decs[8][5] = { "B","S","V","↑","←","→","↓","空格" };
    RussiaDiamonds rd(csl,0,0);
    rd.Open( keys, decs, 1760, 10 );
    MessageDeal( &rd, 1 );
//*/
/* 二个人玩（仅用于测试多窗体，多窗体是为了将来联网作战准备的）
    // 二个人玩时有一个问题：如果两个键同时按着不放，则只有后一个起作用
    Console csl;
    COORD crect = { 38 + 38, 21 };
    csl.Open( "罗刹国方块 alpha", crect );
    int keys[2][8] = { {0x00003B00,0x00003C00,0x00003D00,0x00000057,0x00000041,0x00000044,0x00000053,0x00000058}
                      ,{0x00000037,0x00000038,0x00000039,0x000048E0,0x00004BE0,0x00004DE0,0x000050E0,0x00000030} };
    char decs[2][8][5] = { { "F1", "F2", "F3", "W",  "A",  "D",  "S",  "X" }
                          ,{ "7",  "8",  "9",  "↑", "←", "→", "↓", "0" } };
    RussiaDiamonds rds[2] = { RussiaDiamonds(csl,0,0), RussiaDiamonds(csl,38,0) };
    rds[0].Open( keys[0], decs[0], 1760, 10 );
    rds[1].Open( keys[1], decs[1], 2760, 10 );
    MessageDeal( rds, 2 );
//*/
    return 0;
}
