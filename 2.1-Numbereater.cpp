#define RAYGUI_IMPLEMENTATION
#define SUITSCRW screenW*0.01
#define SUITSCRH screenH*0.01
#define MAXSPLITNUM 2
#include "raylib.h"
#include "raygui.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <cstring>
// 游戏状态
typedef enum { STATE_START, STATE_INPUT, STATE_PLAYING, STATE_GAME_OVER, STATE_GAME_PAUSE } GameState;

// 游戏模式
typedef enum { MODE_CLASSIC, MODE_ADVANCED } GameMode;

// 2D整数坐标
typedef struct Vector2Int {
	int x, y;
} Vector2Int;

// 敌人结构
typedef struct Enemy {
	Vector2Int pos;
	int number;
	int sparesplit;
} Enemy;

// 全局状态
GameState currentState = STATE_START;
GameMode gameMode = MODE_CLASSIC;  // 默认经典模式

// 输入界面相关
char hText[16] = "10";
char wText[16] = "10";
char eText[16] = "8";
bool hEdit = false;
bool wEdit = false;
bool eEdit = false;
bool esEdit= false;
char esText[16]="5";
int baseX ;        // 输入框起始 X 坐标
int baseY ;          // 输入框起始 Y 坐标
int fontSize ;                 // 标签字体大小
int inputWidth ;                  // 输入框宽度
int inputHeight ;                  // 输入框高度
int lineSpacing ;                   // 行间距
int labelOffset1 ;            // "Height:" 和 "Width:" 的 X 偏移
int labelOffset2 ;               // 较长标签的 X 偏移（如 "Amount of Enemies:"）

// 游戏数据
int H = 0, W = 0, E = 0;
int totalCells ;
int zeroCount ;
int* gridOccupant;   // 0:无玩家/敌人, 1:玩家, 2:敌人
int* gridNumber;     // 格子上的数字：0=空, 1=灰色, >=2=玩家/敌人数字
Vector2Int playerPos;
int playerNumber;
Enemy* enemies;      // 动态数组，最大容量 H*W
int enemyCount;
bool gameWin;        // 胜利/失败标志
int dx[8] = { 0, 0, -1, 1, 1,1,-1,-1 };//0上1下2左3右4右下5右上6左上7右下
int dy[8] = { -1, 1, 0, 0, 1,-1,1,-1 };
int screenW; 
int screenH; 
int ave,enemyScore;
float cellSizeX,cellSizeY;
// 敌人自动移动计时器
float enemyMoveTimer = 0.0f;
float ENEMY_MOVE_INTERVAL = 0.3f;  // 敌人每ENEMY_MOVE_INTERVAL秒移动一次
float baseEnemySpeed;               // 玩家输入的初始速度（每秒移动次数）
// 函数声明
bool InitGame();
void UnloadGame();
void DrawStartScreen();
void DrawInputScreen();
void DrawGameScreen();
void DrawGameOverScreen();
void HandlePlayerMove(int dx, int dy);
void MoveEnemies();
void RefreshGrayNumbers();   // 刷新灰色数字（1）
void CheckGameWin();
void DrawGamePause();
int GetWisdom(Vector2Int &p,Vector2Int &ep);
int findNearestGray(Vector2Int start);  // BFS找最近灰色，返回第一步方向（0-3），-1表示未找到
void updateEnemySpeed();                 // 根据玩家得分更新移动间隔
void trySplitEnemies();                  // 尝试分裂敌人（仅高级模式）
bool CheckPointDir(int dir);
void UpdateDrawFrame();
// --------------------------------------------------------------------
// 程序入口
// --------------------------------------------------------------------
int main(void) {
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1000, 600, "Number Eater");
	SetTargetFPS(60);
	GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
	srand((unsigned int)time(NULL));
	while(!WindowShouldClose()){
		UpdateDrawFrame();
	}
	UnloadGame();
	CloseWindow();
	return 0;
}

// --------------------------------------------------------------------
// 开始界面（增加两个模式按钮）
// --------------------------------------------------------------------
void DrawStartScreen() {
	screenW = GetScreenWidth();
	screenH = GetScreenHeight();
	DrawText("Number eater", screenW/2 - MeasureText("Number eater", 100)/2, screenH/2 - 120, 100 , DARKGREEN);
	
	// 经典模式按钮
	if (GuiButton((Rectangle){ screenW/2 - screenW*0.15, screenH/2 - 20, screenW*0.3, screenH*0.06 }, "Classic Mode")) {
		gameMode = MODE_CLASSIC;
		currentState = STATE_INPUT;
	}
	// 高级模式按钮
	if (GuiButton((Rectangle){ screenW/2 - screenW*0.15, screenH/2 + 80, screenW*0.3, screenH*0.06 }, "Advanced Mode")) {
		gameMode = MODE_ADVANCED;
		currentState = STATE_INPUT;
	}
}

// --------------------------------------------------------------------
// 输入界面（不变，仍保留速度输入）
// --------------------------------------------------------------------
void DrawInputScreen() {
	screenW = GetScreenWidth();
	screenH = GetScreenHeight();
	
	// 原有布局计算（保持不变）
	float scale = screenW / 800.0f;
	if (scale < 0.5f) scale = 0.5f;
	if (scale > 2.0f) scale = 2.0f;
	baseX = screenW / 2 - (int)(100 * scale);
	baseY = screenH / 2 - (int)(80 * scale);
	fontSize = (int)(20 * scale);
	inputWidth = (int)(120 * scale);
	inputHeight = (int)(30 * scale);
	lineSpacing = (int)(45 * scale);
	labelOffset1 = (int)(-100 * scale);
	labelOffset2 = (int)(-190 * scale);
	
	int currentY = baseY;   // 用于逐行定位
	
	
	DrawText("Height:", baseX + labelOffset1, currentY + 5, fontSize, BLACK);
	if (GuiTextBox((Rectangle){ (float)baseX, (float)currentY, (float)inputWidth, (float)inputHeight },
		hText, 16, hEdit)) hEdit = !hEdit;
	currentY += lineSpacing;
	
	// 第二行：Width
	DrawText("Width:", baseX + labelOffset1, currentY + 5, fontSize, BLACK);
	if (GuiTextBox((Rectangle){ (float)baseX, (float)currentY, (float)inputWidth, (float)inputHeight },
		wText, 16, wEdit)) wEdit = !wEdit;
	currentY += lineSpacing;
	
	// 第三行：Amount of Enemies
	DrawText("Amount of Enemies:", baseX + labelOffset2, currentY + 5, fontSize, BLACK);
	if (GuiTextBox((Rectangle){ (float)baseX, (float)currentY, (float)inputWidth, (float)inputHeight },
		eText, 16, eEdit)) eEdit = !eEdit;
	currentY += lineSpacing;
	
	// 第四行：Speed of Enemies
	DrawText("Speed of Enemies:", baseX + labelOffset2, currentY + 5, fontSize, BLACK);
	if (GuiTextBox((Rectangle){ (float)baseX, (float)currentY, (float)inputWidth, (float)inputHeight },
		esText, 16, esEdit)) esEdit = !esEdit;
	currentY += lineSpacing;
	
	// ========== 公共部分：开始按钮 ==========
	int buttonWidth = (int)(120 * scale);
	int buttonHeight = (int)(35 * scale);
	currentY += (int)(15 * scale); // 额外间距
	if (GuiButton((Rectangle){ (float)baseX, (float)currentY, (float)buttonWidth, (float)buttonHeight }, "Start Game") || IsKeyPressed(KEY_ENTER)) {
		// 按钮点击处理逻辑（与原来相同）
		H = atoi(hText);
		W = atoi(wText);
		E = atoi(eText);
		baseEnemySpeed = (float)atoi(esText);
		ENEMY_MOVE_INTERVAL = 1.0f / baseEnemySpeed;
		if (H > 0 && W > 0 && E >= 0) {
			if (InitGame()) {
				currentState = STATE_PLAYING;
				enemyMoveTimer = ENEMY_MOVE_INTERVAL;
			} else {
				currentState = STATE_START;
			}
		}

	}
	totalCells = H * W;
}
// --------------------------------------------------------------------
// 游戏初始化（敌人数组改为最大容量 H*W）
// --------------------------------------------------------------------
bool InitGame() {
	if (E >= H * W) E = H * W - 1;
	if (E < 0) E = 0;
	zeroCount=0;
	ave=2;
	enemyScore=E*2;
	gridOccupant = (int*)calloc(H * W, sizeof(int));
	gridNumber = (int*)malloc(H * W * sizeof(int));
	if (!gridOccupant || !gridNumber) return false;
	// 所有格子初始为灰色数字1（无人占据）
	for (int i = 0; i < H * W; ++i) {
		gridOccupant[i] = 0;
		gridNumber[i] = 1;
	}
	
	// 随机生成玩家
	playerPos.x = rand() % W;
	playerPos.y = rand() % H;
	int idx = playerPos.y * W + playerPos.x;
	gridOccupant[idx] = 1;
	gridNumber[idx] = 2;
	playerNumber = 2;
	
	// 分配敌人数组（最大容量 H*W）
	enemies = (Enemy*)malloc(H * W * sizeof(Enemy));
	enemyCount = 0;
	
	// 生成敌人，位置不重合
	for (int i = 0; i < E; ++i) {
		int attempts = 0, maxAttempts = W * H * 2;
		bool placed = false;
		while (!placed && attempts < maxAttempts) {
			int x = rand() % W;
			int y = rand() % H;
			int id = y * W + x;
			if (gridOccupant[id] == 0) {
				enemies[enemyCount].pos.x = x;
				enemies[enemyCount].pos.y = y;
				enemies[enemyCount].number = 2;
				enemies[enemyCount].sparesplit = MAXSPLITNUM;
				gridOccupant[id] = 2;
				gridNumber[id] = 2;
				enemyCount++;
				placed = true;
			}
			attempts++;
		}
		if (!placed) break;
	}
	E = enemyCount;
	return true;
}

// --------------------------------------------------------------------
// 卸载游戏数据
// --------------------------------------------------------------------
void UnloadGame() {
	if (gridOccupant) { free(gridOccupant); gridOccupant = NULL; }
	if (gridNumber)   { free(gridNumber);   gridNumber = NULL; }
	if (enemies)      { free(enemies);      enemies = NULL; }
	enemyCount = 0;
}

// --------------------------------------------------------------------
// 游戏进行界面
// --------------------------------------------------------------------
void DrawGameScreen() {
	screenW = GetScreenWidth();
	screenH = GetScreenHeight();
	if(enemyCount!=0)ave=enemyScore/enemyCount;
	// 计算单元格尺寸（占满整个屏幕）
	cellSizeX = (float)screenW/(float)W;
	cellSizeY = (float)screenH/(float)H;	

	// ----- 玩家移动输入（回合制）-----
	if (IsKeyPressed(KEY_W)||CheckPointDir(0)) HandlePlayerMove(0, -1);
	if (IsKeyPressed(KEY_S)||CheckPointDir(1)) HandlePlayerMove(0, 1);
	if (IsKeyPressed(KEY_A)||CheckPointDir(2)) HandlePlayerMove(-1, 0);
	if (IsKeyPressed(KEY_D)||CheckPointDir(3)) HandlePlayerMove(1, 0);
	if (IsKeyPressed(KEY_P)||GetGestureDetected() == GESTURE_PINCH_IN) currentState=STATE_GAME_PAUSE;
	
	// ----- 敌人自动移动（基于时间）-----
	enemyMoveTimer -= GetFrameTime();
	if (enemyMoveTimer <= 0.0f) {
		MoveEnemies();
		enemyMoveTimer = ENEMY_MOVE_INTERVAL;
		// 敌人移动后刷新灰色数字
		RefreshGrayNumbers();
		// 高级模式下尝试分裂
		if (gameMode == MODE_ADVANCED) {
			trySplitEnemies();
		}
	}
	
	// ----- 绘制网格 -----
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			int idx = y * W + x;
			Rectangle rec = {  x * cellSizeX,  y * cellSizeY, cellSizeX, cellSizeY };
			
			Color color;
			if (gridOccupant[idx] == 1) color = RED;          // 玩家：红色
			else if (gridOccupant[idx] == 2) color = SKYBLUE;    // 敌人：蓝色
			else { // 无玩家/敌人
				if (gridNumber[idx] == 1) color = LIGHTGRAY;  // 灰色数字1
				else color = WHITE;                           // 空白（数字0）
			}
			DrawRectangleRec(rec, color);
			DrawRectangleLinesEx(rec, 1, DARKGRAY);
			
			// 显示数字（只有非0才显示）
			if (gridNumber[idx] > 0) {
				char numStr[8];
				sprintf(numStr, "%d", gridNumber[idx]);
				int fontSize = std::min(cellSizeX,cellSizeY) / 2;
				if (fontSize < 8) fontSize = 8;
				int textWidth = MeasureText(numStr, fontSize);
				DrawText(numStr, rec.x + (cellSizeX - textWidth) / 2, rec.y + (cellSizeY - fontSize) / 2, fontSize, WHITE);
			}
		}
	}
}

// --------------------------------------------------------------------
// 玩家移动处理（增加速度更新）
// --------------------------------------------------------------------
void HandlePlayerMove(int dx, int dy) {
	int newX = playerPos.x + dx;
	int newY = playerPos.y + dy;
	if (newX < 0 || newX >= W || newY < 0 || newY >= H) return;
	
	int playerIdx = playerPos.y * W + playerPos.x;
	int targetIdx = newY * W + newX;
	
	// 情况1：目标格子是灰色数字（1）
	if (gridOccupant[targetIdx] == 0 && gridNumber[targetIdx] == 1) {
		playerNumber++;                     // 玩家数字+1
		gridNumber[targetIdx] = 0;          // 灰色数字被吞噬 → 变为0（空）
		gridNumber[playerIdx] = playerNumber; // 更新玩家所在格子的数字
		zeroCount++;
		// 玩家位置不变
		// 更新速度（高级模式）
		if (gameMode == MODE_ADVANCED) updateEnemySpeed();
		return;
	}
	
	// 情况2：目标格子是空（0）
	if (gridOccupant[targetIdx] == 0 && gridNumber[targetIdx] == 0) {
		// 原玩家格子变空
		gridOccupant[playerIdx] = 0;
		gridNumber[playerIdx] = 0;
		// 移动玩家
		playerPos.x = newX; playerPos.y = newY;
		gridOccupant[targetIdx] = 1;
		gridNumber[targetIdx] = playerNumber;
		return;
	}
	
	// 情况3：目标格子是敌人
	if (gridOccupant[targetIdx] == 2) {
		for (int i = 0; i < enemyCount; ++i) {
			if (enemies[i].pos.x == newX && enemies[i].pos.y == newY) {
				int enemyNum = enemies[i].number;
				if (playerNumber > enemyNum) {
					// 玩家吞噬敌人
					playerNumber += enemyNum;
					enemyScore-=enemyNum;
					// 移除敌人
					enemies[i] = enemies[enemyCount - 1];
					enemyCount--;
					// 目标格子变为玩家
					gridOccupant[targetIdx] = 1;
					gridNumber[targetIdx] = playerNumber;
					// 原玩家格子变空
					gridOccupant[playerIdx] = 0;
					gridNumber[playerIdx] = 0;
					playerPos.x = newX; playerPos.y = newY;
					// 更新速度（高级模式）
					if (gameMode == MODE_ADVANCED) updateEnemySpeed();
				} else {
					// 玩家被吞噬，游戏失败
					gameWin = false;
					currentState = STATE_GAME_OVER;
				}
				break;
			}
		}
		return;
	}
	// 其他情况（玩家自己的格子）不做处理
}

// --------------------------------------------------------------------
// 敌人自动移动（使用BFS觅食）
// --------------------------------------------------------------------
void MoveEnemies() {
	for (int i = 0; i < enemyCount; ++i) {
		int dir = GetWisdom(playerPos, enemies[i].pos); // 返回0-7，但只取前4个方向（上下左右）
		int dxMove = 0, dyMove = 0;
		if (dir == 0 || dir == 5 || dir == 6) dyMove = -1;
		else if (dir == 1 || dir == 4 || dir == 7) dyMove = 1;
		else if (dir == 2) dxMove = -1;
		else if (dir == 3) dxMove = 1;
		int nx = enemies[i].pos.x + dxMove;
		int ny = enemies[i].pos.y + dyMove;
		if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue; // 越界则不移动
		
		int oldIdx = enemies[i].pos.y * W + enemies[i].pos.x;
		int newIdx = ny * W + nx;
		
		// 目标格子是灰色数字（1）
		if (gridOccupant[newIdx] == 0 && gridNumber[newIdx] == 1) {
			enemies[i].number++;            // 敌人数值+1
			enemyScore++;
			gridNumber[newIdx] = 0;         // 灰色数字被吞噬 → 变空
			gridNumber[oldIdx] = enemies[i].number; // 更新敌人所在格子的数字
			zeroCount++;
		// 敌人位置不变
			continue;
		}
		
		// 目标格子是空（0）
		if (gridOccupant[newIdx] == 0 && gridNumber[newIdx] == 0) {
			// 原格子变空
			gridOccupant[oldIdx] = 0;
			gridNumber[oldIdx] = 0;
			// 移动敌人
			enemies[i].pos.x = nx;
			enemies[i].pos.y = ny;
			gridOccupant[newIdx] = 2;
			gridNumber[newIdx] = enemies[i].number;
			continue;
		}
		
		// 目标格子是玩家
		if (gridOccupant[newIdx] == 1) {
			if (enemies[i].number >= playerNumber) {
				// 敌人吞噬玩家，游戏失败
				gameWin = false;
				currentState = STATE_GAME_OVER;
				return;
			} else {
				// 玩家吞噬该敌人
				playerNumber += enemies[i].number;
				// 目标格子（玩家格子）数字更新
				gridNumber[newIdx] = playerNumber;
				// 移除敌人
				enemies[i] = enemies[enemyCount - 1];
				enemyCount--;
				i--; // 重新检查当前索引
				// 敌人原格子变空（已移除）
				gridOccupant[oldIdx] = 0;
				gridNumber[oldIdx] = 0;
				// 更新速度（高级模式）
				if (gameMode == MODE_ADVANCED) updateEnemySpeed();
				continue;
			}
		}
		
		// 目标格子是其他敌人 → 不移动
	}
	
	// 敌人移动后检查胜利
	CheckGameWin();
}

// --------------------------------------------------------------------
// 刷新灰色数字：当0的数量 > 总格子数/4时，随机在一个0位置生成1
// --------------------------------------------------------------------
void RefreshGrayNumbers() {
	if (zeroCount >= totalCells / 4) {
		int attempts = 0, maxAttempts = totalCells * 2;
		while (attempts < maxAttempts) {
			int idx = rand() % totalCells;
			if (gridOccupant[idx] == 0 && gridNumber[idx] == 0) {
				gridNumber[idx] = 1;
				break;
			}
			attempts++;
		}
	}
}

// --------------------------------------------------------------------
// 检查游戏胜利
// --------------------------------------------------------------------
void CheckGameWin() {
	if (enemyCount <= 0) {
		gameWin = true;
		currentState = STATE_GAME_OVER;
	}
}

// --------------------------------------------------------------------
// 游戏结束界面
// --------------------------------------------------------------------
void DrawGameOverScreen() {
	screenW = GetScreenWidth();
	screenH = GetScreenHeight();
	const char* message = gameWin ? "VICTORY!" : "DEFEAT!";
	char FinalScore[20];
	Color msgColor = gameWin ? DARKGREEN : MAROON;
	DrawText(message, screenW/2 - MeasureText(message, 50)/2, screenH/2 - 60, 50, msgColor);
	DrawText("Score:",screenW/2 - MeasureText("Score:", 30)/2-50, screenH/2 - 10,30,BLACK);
	sprintf(FinalScore, "%d", playerNumber);
	DrawText(FinalScore,screenW/2+30,screenH/2 - 10,30,BLACK);
	if (GuiButton((Rectangle){ screenW/2 - 80, screenH/2 + 80, 160, 40 }, "Play Again")||IsKeyPressed(KEY_ENTER)) {
		UnloadGame();
		currentState = STATE_START;
	}
}

// --------------------------------------------------------------------
// 智能决策函数（整合BFS觅食、追逐、逃跑）
// --------------------------------------------------------------------
int GetWisdom(Vector2Int &p, Vector2Int &ep) {
	int enemyNum = gridNumber[ep.y * W + ep.x];
	int playerNum = playerNumber;
	int currentDist = abs(p.x - ep.x) + abs(p.y - ep.y);
	
	// 决策优先级：
	// 1. 如果敌人 >= 玩家：追逐玩家（使用曼哈顿方向）
	// 2. 如果敌人 < 玩家：
	//    a) 距离 < 20：逃跑（使距离增大）
	//    b) 距离 >= 20：使用BFS觅食（找最近灰色1）
	
	if (enemyNum >= playerNum) {
		// 追逐玩家：选择使曼哈顿距离减小的方向（优先考虑能吃灰色1的方向）
		int bestDir = -1;
		int minDist = currentDist;
		for (int dir = 0; dir < 4; dir++) {
			int nx = ep.x + dx[dir];
			int ny = ep.y + dy[dir];
			if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
			int idx = ny * W + nx;
			if (gridOccupant[idx] == 2) continue; // 其他敌人阻挡
			int newDist = abs(p.x - nx) + abs(p.y - ny);
			if (newDist < minDist) {
				minDist = newDist;
				bestDir = dir;
			}
		}
		if (bestDir != -1) return bestDir;
	} else {
		if (currentDist <= 20) {
			// 逃跑：选择使曼哈顿距离增大的方向
			int bestDir = -1;
			int maxDist = currentDist;
			for (int dir = 0; dir < 4; dir++) {
				int nx = ep.x + dx[dir];
				int ny = ep.y + dy[dir];
				if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
				int idx = ny * W + nx;
				if (gridOccupant[idx] == 2) continue;
				int newDist = abs(p.x - nx) + abs(p.y - ny);
				if (newDist > maxDist) {
					maxDist = newDist;
					bestDir = dir;
				}
			}
			if (bestDir != -1) return bestDir;
		} else {
			// 觅食：BFS找最近灰色1
			int dir = findNearestGray(ep);
			if (dir != -1) return dir;
		}
	}
	
	// 没有合适方向，随机选一个可行方向
	int validDirs[4], validCount = 0;
	for (int dir = 0; dir < 4; dir++) {
		int nx = ep.x + dx[dir];
		int ny = ep.y + dy[dir];
		if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
		int idx = ny * W + nx;
		if (gridOccupant[idx] != 2) validDirs[validCount++] = dir;
	}
	if (validCount > 0) return validDirs[GetRandomValue(0, validCount - 1)];
	
	return GetRandomValue(0, 3); // 无路可走，返回随机
}

// --------------------------------------------------------------------
// BFS找最近的灰色数字1，返回第一步方向（0-3），-1表示未找到
// --------------------------------------------------------------------
int findNearestGray(Vector2Int start) {
	int totalCells = H * W;
	int* visited = (int*)calloc(totalCells, sizeof(int));
	int* queueX = (int*)malloc(totalCells * sizeof(int));
	int* queueY = (int*)malloc(totalCells * sizeof(int));
	int* prevDir = (int*)malloc(totalCells * sizeof(int)); // 记录从起点到每个格子的第一步方向
	
	int front = 0, rear = 0;
	queueX[rear] = start.x;
	queueY[rear] = start.y;
	visited[start.y * W + start.x] = 1;
	prevDir[start.y * W + start.x] = -1;
	rear++;
	
	int foundIdx = -1;
	while (front < rear) {
		int x = queueX[front];
		int y = queueY[front];
		front++;
		int idx = y * W + x;
		
		// 如果是灰色数字1且不是起点
		if (gridNumber[idx] == 1 && !(x == start.x && y == start.y)) {
			foundIdx = idx;
			break;
		}
		
		// 探索四个方向
		for (int d = 0; d < 4; d++) {
			int nx = x + dx[d];
			int ny = y + dy[d];
			if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
			int nidx = ny * W + nx;
			if (visited[nidx]) continue;
			// 可走条件：不是其他敌人（gridOccupant != 2），且可以是空格、灰色1或玩家（玩家格子可进入战斗）
			if (gridOccupant[nidx] == 2) continue; // 其他敌人阻挡
			if (abs(nx - playerPos.x) + abs(ny - playerPos.y)<=20)continue;
			visited[nidx] = 1;
			queueX[rear] = nx;
			queueY[rear] = ny;
			// 记录方向：如果是第一次扩展，则保存从起点到该格子的第一步方向
			if (x == start.x && y == start.y) {
				prevDir[nidx] = d; // 第一步的方向
			} else {
				prevDir[nidx] = prevDir[idx]; // 继承父节点的第一步方向
			}
			rear++;
		}
	}
	
	int result = -1;
	if (foundIdx != -1) {
		result = prevDir[foundIdx];
	}
	
	free(visited);
	free(queueX);
	free(queueY);
	free(prevDir);
	return result;
}

// --------------------------------------------------------------------
// 更新敌人移动速度（高级模式）
// 玩家每增加10分，速度增加0.05
// --------------------------------------------------------------------
void updateEnemySpeed() {
	int scoreIncrease = (playerNumber - 2) / 10;  // 玩家初始2，每10分增加一次
	float newSpeed = baseEnemySpeed + 0.1f * scoreIncrease;
	if (newSpeed < 0.1f) newSpeed = 0.1f; // 最小速度限制
	if (newSpeed > 12.0f) newSpeed=12.0f; //最大速度限制
	ENEMY_MOVE_INTERVAL = 1.0f / newSpeed;
}

// --------------------------------------------------------------------
// 尝试分裂敌人（高级模式）
// 条件：敌人分数 > 2 * 玩家分数 or 敌人与玩家的曼哈顿距离<=5且分裂后敌人分数>=原分数1/4
// --------------------------------------------------------------------
void trySplitEnemies() {
	bool splitHappened = true;
	int newNum1 ;
	int newNum2 ;
	int candidates[5][2];
	int candCount = 0;
	int ox,oy ;
	int oIdx ;
	int Sparesplit;
	while (splitHappened) {
		splitHappened = false;
		for (int i = 0; i < enemyCount; i++) {
			Sparesplit=enemies[i].sparesplit;
			newNum1 = enemies[i].number / 2;
			newNum2 = enemies[i].number - newNum1; // 保证总和不变
			if (enemies[i].number > 2 * playerNumber || (abs(playerPos.x - enemies[i].pos.x) + abs(playerPos.y - enemies[i].pos.y)<=5 && enemies[i].number<playerNumber && Sparesplit>0 &&newNum1>=8)) {
				// 准备分裂
				// 收集可放置的空格：原格子 + 四个相邻空格
				candCount = 0;
				// 原格子
				ox = enemies[i].pos.x, oy = enemies[i].pos.y;
				oIdx = oy * W + ox;
				if (gridOccupant[oIdx] == 2) { // 当前是敌人，即将移除
					candidates[candCount][0] = ox;
					candidates[candCount][1] = oy;
					candCount++;
				}
				// 四个方向
				for (int d = 0; d < 4; d++) {
					int nx = ox + dx[d];
					int ny = oy + dy[d];
					if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
					int nidx = ny * W + nx;
					if (gridOccupant[nidx] == 0 && gridNumber[nidx] == 0) {
						candidates[candCount][0] = nx;
						candidates[candCount][1] = ny;
						candCount++;
					}
				}
				
				if (candCount >= 2) {
					// 移除原敌人
					gridOccupant[oIdx] = 0;
					gridNumber[oIdx] = 0;
					// 用最后一个元素覆盖当前
					enemies[i] = enemies[enemyCount - 1];
					enemyCount--;
					
					// 随机选两个不同位置
					int pos1 = GetRandomValue(0, candCount - 1);
					int pos2;
					do {
						pos2 = GetRandomValue(0, candCount - 1);
					} while (pos2 == pos1 && candCount > 1);
					
					// 添加第一个新敌人
					enemies[enemyCount].pos.x = candidates[pos1][0];
					enemies[enemyCount].pos.y = candidates[pos1][1];
					enemies[enemyCount].number = newNum1;
					enemies[enemyCount].sparesplit = Sparesplit-1;
					gridOccupant[enemies[enemyCount].pos.y * W + enemies[enemyCount].pos.x] = 2;
					gridNumber[enemies[enemyCount].pos.y * W + enemies[enemyCount].pos.x] = newNum1;
					enemyCount++;
					
					// 添加第二个新敌人
					enemies[enemyCount].pos.x = candidates[pos2][0];
					enemies[enemyCount].pos.y = candidates[pos2][1];
					enemies[enemyCount].number = newNum2;
					enemies[enemyCount].sparesplit = Sparesplit-1;
					gridOccupant[enemies[enemyCount].pos.y * W + enemies[enemyCount].pos.x] = 2;
					gridNumber[enemies[enemyCount].pos.y * W + enemies[enemyCount].pos.x] = newNum2;
					enemyCount++;
					
					splitHappened = true;
					break; // 跳出循环，重新从头检查（因为数组已变）
				}
			}
		}
	}
}

// --------------------------------------------------------------------
// 暂停界面
// --------------------------------------------------------------------
void DrawGamePause(){
	ClearBackground(WHITE);
	DrawText("Pause", screenW/2 - MeasureText("Pause", 50)/2, screenH/2 - 80, 40, BLACK );
	if(GuiButton((Rectangle){ screenW/2 - 80, screenH/2 + 20, 160, 40 }, "Restart")){
		UnloadGame();
		currentState = STATE_START;
	}
	if(GuiButton((Rectangle){ screenW/2 - 80, screenH/2 + 70, 160, 40 }, "Back")|| IsKeyPressed(KEY_P)){
		currentState = STATE_PLAYING;
	}
}
bool CheckPointDir(int dir){//0123上下左右
	if(!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))return false;
		Vector2 ClickPos=GetMousePosition();
	//计算相对坐标
	float dx=ClickPos.x-(playerPos.x*cellSizeX+cellSizeX/2);
	float dy=(playerPos.y*cellSizeY+cellSizeY/2)-ClickPos.y;
//	printf("%f %f\n",dx,dy);
	switch(dir){ 
	case 0:
		if(dx<dy&&dx>-dy)return true;
		break;
	case 1:
		if(dx<-dy&& dx>dy)return true;
		break;
	case 2:
		if(dy<-dx&&dy>dx)return true;
	//	printf("yes\n");
		break;
	case 3:
		if(dy<dx&&dx>-dy)return true;
		break;
	}
	return false;
}
void UpdateDrawFrame() {
	BeginDrawing();
	ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
	switch (currentState){
		case STATE_START:    DrawStartScreen(); break;
		case STATE_INPUT:    DrawInputScreen(); break;
		case STATE_PLAYING:  DrawGameScreen(); break;
		case STATE_GAME_OVER:DrawGameOverScreen(); break;
		case STATE_GAME_PAUSE: DrawGamePause(); break;
	}
	EndDrawing();
}
