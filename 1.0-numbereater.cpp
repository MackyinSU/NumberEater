#define RAYGUI_IMPLEMENTATION
#define SUITSCRW screenW*0.01
#define SUITSCRH screenH*0.01
#define SUITSIZE std::max(SUITSCRW,SUITSCRH)
#include "raylib.h"
#include "raygui.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

// 游戏状态
typedef enum { STATE_START, STATE_INPUT, STATE_PLAYING, STATE_GAME_OVER, STATE_GAME_PAUSE } GameState;

// 2D整数坐标
typedef struct Vector2Int {
	int x, y;
} Vector2Int;

// 敌人结构
typedef struct Enemy {
	Vector2Int pos;
	int number;
} Enemy;

// 全局状态
GameState currentState = STATE_START;

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
int labelOffset2 ;               // 较长标签的 X 偏移（如 "Amount of Enemies:"
// 游戏数据
int H = 0, W = 0, E = 0;
int* gridOccupant;   // 0:无玩家/敌人, 1:玩家, 2:敌人
int* gridNumber;     // 格子上的数字：0=空, 1=灰色, >=2=玩家/敌人数字
Vector2Int playerPos;
int playerNumber;
Enemy* enemies;
int enemyCount;
bool gameWin;        // 胜利/失败标志
int dx[8] = { 0, 0, -1, 1, 1,1,-1,-1 };//0上1下2左3右4右下5右上6左上7右下
int dy[8] = { -1, 1, 0, 0, 1,-1,1,-1 };
int screenW; 
int screenH; 
// 敌人自动移动计时器
float enemyMoveTimer = 0.0f;
float ENEMY_MOVE_INTERVAL = 0.3f;  // 敌人每ENEMY_MOVE_INTERVAL秒移动一次

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

// --------------------------------------------------------------------
// 程序入口
// --------------------------------------------------------------------
int main(void) {
	
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
	InitWindow(800, 600, "Number Eater");
	SetTargetFPS(60);
	GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
	srand((unsigned int)time(NULL));
	while(!WindowShouldClose()){
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
	
	UnloadGame();
	CloseWindow();
	return 0;
}

// --------------------------------------------------------------------
// 开始界面
// --------------------------------------------------------------------
void DrawStartScreen() {
	screenW = GetScreenWidth();
	screenH = GetScreenHeight();
	DrawText("Number eater", screenW/2 - MeasureText("Number eater", 100)/2, screenH/2 - 80, 100 , DARKGREEN);
	if (GuiButton((Rectangle){ screenW/2 - screenW*0.1, screenH/2 + screenH*0.025, screenW*0.2, screenH*0.05 }, "Game Start")) {
		currentState = STATE_INPUT;
	}
}

// --------------------------------------------------------------------
// 输入界面
// --------------------------------------------------------------------
void DrawInputScreen() {
	screenW = GetScreenWidth();
	screenH = GetScreenHeight();
	// 以宽度 800 像素为基准计算缩放因子（可根据需要调整基准值）
	float scale = screenW / 800.0f;
	// 限制缩放范围，避免过大或过小导致界面异常
	if (scale < 0.5f) scale = 0.5f;
	if (scale > 2.0f) scale = 2.0f;
	// 根据缩放因子计算动态尺寸
	baseX = screenW / 2 - (int)(100 * scale);          // 输入框起始 X 坐标
	baseY = screenH / 2 - (int)(80 * scale);           // 输入框起始 Y 坐标
	fontSize = (int)(20 * scale);                      // 标签字体大小
	inputWidth = (int)(120 * scale);                   // 输入框宽度
	inputHeight = (int)(30 * scale);                   // 输入框高度
	lineSpacing = (int)(45 * scale);                   // 行间距
	labelOffset1 = (int)(-100 * scale);                // "Height:" 和 "Width:" 的 X 偏移
	labelOffset2 = (int)(-190 * scale);                // 较长标签的 X 偏移（如 "Amount of Enemies:"）
	
	// 第一行：Height
	DrawText("Height:", baseX + labelOffset1, baseY + 5, fontSize, BLACK);
	if (GuiTextBox((Rectangle){ (float)baseX, (float)baseY, (float)inputWidth, (float)inputHeight },
		hText, 16, hEdit)) hEdit = !hEdit;
	baseY += lineSpacing;
	
	// 第二行：Width
	DrawText("Width:", baseX + labelOffset1, baseY + 5, fontSize, BLACK);
	if (GuiTextBox((Rectangle){ (float)baseX, (float)baseY, (float)inputWidth, (float)inputHeight },
		wText, 16, wEdit)) wEdit = !wEdit;
	baseY += lineSpacing;
	
	// 第三行：Amount of Enemies（标签较长，使用更大的左偏移）
	DrawText("Amount of Enemies:", baseX + labelOffset2, baseY + 5, fontSize, BLACK);
	if (GuiTextBox((Rectangle){ (float)baseX, (float)baseY, (float)inputWidth, (float)inputHeight },
		eText, 16, eEdit)) eEdit = !eEdit;
	baseY += lineSpacing;
	
	// 第四行：Speed of Enemies
	DrawText("Speed of Enemies:", baseX + labelOffset2, baseY + 5, fontSize, BLACK);
	if (GuiTextBox((Rectangle){ (float)baseX, (float)baseY, (float)inputWidth, (float)inputHeight },
		esText, 16, esEdit)) esEdit = !esEdit;
	baseY += lineSpacing;
	
	// “Start Game” 按钮（使用相同的缩放因子）
	int buttonWidth = (int)(120 * scale);
	int buttonHeight = (int)(35 * scale);
	baseY += (int)(15 * scale); // 额外间距
	if (GuiButton((Rectangle){ (float)baseX, (float)baseY, (float)buttonWidth, (float)buttonHeight },"Start Game")) {
		// 按钮点击处理逻辑（保持不变）
		H = atoi(hText);
		W = atoi(wText);
		E = atoi(eText);
		ENEMY_MOVE_INTERVAL=1.0f/(float)atoi(esText);
		if (H > 0 && W > 0 && E >= 0) {
			if (InitGame()) {
				currentState = STATE_PLAYING;
				enemyMoveTimer = ENEMY_MOVE_INTERVAL;
			} else {
				currentState = STATE_START;
			}
		}
	}
}

// --------------------------------------------------------------------
// 游戏初始化
// --------------------------------------------------------------------
bool InitGame() {
	if (E >= H * W) E = H * W - 1;
	if (E < 0) E = 0;
	
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
	
	// 分配敌人数组
	enemies = (Enemy*)malloc(E * sizeof(Enemy));
	enemyCount = 0;
	
	// 生成敌人，位置不重合
	for (int i = 0; i < E; ++i) {
		int attempts = 0, maxAttempts = W * H * 2;
		bool placed = false;
		while (!placed && attempts < maxAttempts) {
			int x = rand() % W;
			int y = rand() % H;
			int id = y * W + x;
			if (gridOccupant[id] == 0) { // 且不为玩家（gridOccupant[id]==0）
				enemies[enemyCount].pos.x = x;
				enemies[enemyCount].pos.y = y;
				enemies[enemyCount].number = 2;
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
// 游戏进行界面（绘制网格 + 玩家输入 + 敌人自动移动）
// --------------------------------------------------------------------
void DrawGameScreen() {
	int screenW = GetScreenWidth();
	int screenH = GetScreenHeight();
	
	// 计算单元格尺寸（正方形，居中）
	float cellSizeX = (float)screenW/(float)W;//(screenW / W < screenH / H) ? screenW / W : screenH / H;
	float cellSizeY = (float)screenH/(float)H;	

	// ----- 玩家移动输入（回合制）-----
	if (IsKeyPressed(KEY_W)) HandlePlayerMove(0, -1);
	if (IsKeyPressed(KEY_S)) HandlePlayerMove(0, 1);
	if (IsKeyPressed(KEY_A)) HandlePlayerMove(-1, 0);
	if (IsKeyPressed(KEY_D)) HandlePlayerMove(1, 0);
	if (IsKeyPressed(KEY_P)) currentState=STATE_GAME_PAUSE;
	// ----- 敌人自动移动（基于时间）-----
	enemyMoveTimer -= GetFrameTime();
	if (enemyMoveTimer <= 0.0f) {
		MoveEnemies();
		enemyMoveTimer = ENEMY_MOVE_INTERVAL;
		// 敌人移动后刷新灰色数字
		RefreshGrayNumbers();
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
// 玩家移动处理（完全重写）
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
		// 玩家位置不变
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
// 敌人自动移动（每帧由计时器触发）
// --------------------------------------------------------------------
void MoveEnemies() {
	for (int i = 0; i < enemyCount; ++i) {
		int dir = GetWisdom(playerPos,enemies[i].pos); //0上1下2左3右4右下5右上6左上7左下
		int dx = 0, dy = 0;
		if (dir == 0||dir==5||dir==6) dy = -1;
		else if (dir == 1||dir==4||dir==7) dy = 1;
		else if (dir == 2) dx = -1;
		else if (dir == 3)dx = 1;
		int nx = enemies[i].pos.x + dx;
		int ny = enemies[i].pos.y + dy;
		if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue; // 越界则不移动
		
		int oldIdx = enemies[i].pos.y * W + enemies[i].pos.x;
		int newIdx = ny * W + nx;
		
		// 目标格子是灰色数字（1）
		if (gridOccupant[newIdx] == 0 && gridNumber[newIdx] == 1) {
			enemies[i].number++;            // 敌人数值+1
			gridNumber[newIdx] = 0;         // 灰色数字被吞噬 → 变空
			gridNumber[oldIdx] = enemies[i].number; // 更新敌人所在格子的数字
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
				continue;
			}
		}
		
		// 目标格子是其他敌人 → 不移动
	}
	
	// 敌人移动后检查胜利
	CheckGameWin();
}

// --------------------------------------------------------------------
// 刷新灰色数字：当0的数量 > 总格子数的一半时，随机在一个0位置生成1
// --------------------------------------------------------------------
void RefreshGrayNumbers() {
	int totalCells = H * W;
	int zeroCount = 0;
	// 统计值为0的格子数量
	for (int i = 0; i < totalCells; ++i) {
		if (gridOccupant[i] == 0 && gridNumber[i] == 0) zeroCount++;
	}
	
	if (zeroCount > totalCells / 3) {
		// 随机找一个值为0的格子
		int attempts = 0, maxAttempts = totalCells * 2;
		while (attempts < maxAttempts) {
			int idx = rand() % totalCells;
			if (gridOccupant[idx] == 0 && gridNumber[idx] == 0) {
				gridNumber[idx] = 1; // 刷新灰色数字
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
	int screenW = GetScreenWidth();
	int screenH = GetScreenHeight();
	const char* message = gameWin ? "VICTORY!" : "DEFEAT!";
	Color msgColor = gameWin ? DARKGREEN : MAROON;
	DrawText(message, screenW/2 - MeasureText(message, 50)/2, screenH/2 - 60, 50, msgColor);
	if (GuiButton((Rectangle){ screenW/2 - 80, screenH/2 + 40, 160, 40 }, "Play Again")) {
		UnloadGame();
		currentState = STATE_START;
	}
}
int GetWisdom(Vector2Int &p,Vector2Int &ep){//0123 上下左右
	// 获取当前敌人与玩家的数值（需要访问全局变量）
	int enemyNum = gridNumber[ep.y * W + ep.x];
	int playerNum = playerNumber;
	int currentDist = abs(playerPos.x - ep.x) + abs(playerPos.y - ep.y);
	int validDirs[4] = { 0 };      // 存储满足距离条件的方向
	int validCount = 0;
	int eatGrayDir = -1;          // 记录可以吃到灰色1的方向（同时满足距离条件）
	// 遍历四个方向
	for (int dir = 0; dir < 8; dir++) {
		int nx = ep.x + dx[dir];
		int ny = ep.y + dy[dir];
		// 边界检查
		if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
		int idx = ny * W + nx;
		// 检查目标格子是否可进入（不能是其他敌人，不能是越界）
		if (gridOccupant[idx] == 2) continue; // 其他敌人阻挡
		// 计算移动后的曼哈顿距离
		int newDist = abs(playerPos.x - nx) + abs(playerPos.y - ny);
		// 根据数值决定期望的距离变化
			if(enemyNum >= playerNum) {
				// 靠近玩家：新距离 < 当前距离
				if (newDist < currentDist) {
					validDirs[validCount++] = dir;
					// 如果这个格子是灰色数字1，记录优先方向
					if (gridOccupant[idx] == 0 && gridNumber[idx] == 1) {
						eatGrayDir = dir;   // 后覆盖，取最后一个，可自行改为只取第一个
					}
				}
			}else if(currentDist>15){
				if ( gridNumber[idx] == 1) {
					eatGrayDir = dir;   // 后覆盖，取最后一个，可自行改为只取第一个
				}
			}else{
				// 远离玩家：新距离 > 当前距离
				if (newDist > currentDist) {
					validDirs[validCount++] = dir;
					if (gridOccupant[idx] == 0 && gridNumber[idx] == 1) {
						eatGrayDir = dir;   // 后覆盖，取最后一个，可自行改为只取第一个
					}
				}
			}
		
	}
	
	// 1. 如果是靠近模式，并且存在能吃灰色1的方向，优先返回该方向
	if (enemyNum >= playerNum && eatGrayDir != -1) {
		return eatGrayDir;
	}
	
	// 2. 如果有任何满足距离条件的方向，随机选择一个
	if (validCount > 0) {
		return validDirs[GetRandomValue(0, validCount - 1)];
	}
	// 3. 否则，从所有可移动方向（不越界、不是其他敌人）中随机选一个
	int anyValid[4], anyCount = 0;
	for (int dir = 0; dir < 4; dir++) {
		int nx = ep.x + dx[dir];
		int ny = ep.y + dy[dir];
		if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
		int idx = ny * W + nx;
		if (gridOccupant[idx] != 2) { // 不是其他敌人
			anyValid[anyCount++] = dir;
		}
	}
	if (anyCount > 0) {
		return anyValid[GetRandomValue(0, anyCount - 1)];
	}
	// 4. 无路可走，随机返回一个方向（调用者会处理越界）
	return GetRandomValue(0, 3);
}
void DrawGamePause(){
	ClearBackground(WHITE);
	DrawText("Pause", screenW/2 - MeasureText("Pause", 50)/2, screenH/2 - 80, 40, BLACK );
	if(GuiButton((Rectangle){ screenW/2 - 80, screenH/2 + 20, 160, 40 }, "Restart")){
		currentState = STATE_START;
	}
	if(GuiButton((Rectangle){ screenW/2 - 80, screenH/2 + 70, 160, 40 }, "Back")){
		currentState = STATE_PLAYING;
	}
}
