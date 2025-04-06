#include <nds.h>
#include <stdio.h>
#include <time.h>

PrintConsole topScreen;
PrintConsole bottomScreen;

const uint8_t HOR_OFFSET = 0;
const uint8_t VER_OFFSET = 0;
const uint8_t COLS = (31 + HOR_OFFSET);
const uint8_t ROWS = (23 + VER_OFFSET);

bool Resume = false;
bool GenBall = true;
bool Start = false;
bool PressedButton = false;
bool BallEaten = false;

int BallX, BallY, ANSBallX, ANSBallY;
int SnakeX = (COLS/2) + 4;
int SnakeY = ROWS/2;
int VSnakeX = 1;
int VSnakeY = 0;
int Lifes = 3;
int Score = 0;
int SnakeLength = 2;
int counter = 0;
int SnakePOSbuffer[6000][2];

static void CheckInput() {
	scanKeys();
	int pressed = keysDown();
	
	if (pressed & KEY_START) {
		exit(0);
	} else if ((pressed & KEY_UP) && (VSnakeX != 0 || !Start) && !PressedButton) {
		PressedButton = true;
		if (!Start) {
			Start = true;
		}
		VSnakeY = -1;
		VSnakeX = 0;
	} else if ((pressed & KEY_DOWN) && (VSnakeX != 0 || !Start) && !PressedButton) {
		PressedButton = true;
		if (!Start) {
			Start = true;
		}
		VSnakeY = 1;
		VSnakeX = 0;
	} else if ((pressed & KEY_LEFT) && (VSnakeY != 0 || !Start) && !PressedButton) {
		PressedButton = true;
		if (!Start) {
			Start = true;
		}
		VSnakeX = -1;
		VSnakeY = 0;
	} else if ((pressed & KEY_RIGHT) && (VSnakeY != 0 || !Start) && !PressedButton) {
		PressedButton = true;
		if (!Start) {
			Start = true;
		}
		VSnakeX = 1;
		VSnakeY = 0;
	}
}

void sleep(float delay) {
	float ticks = delay/20;
	float start = 0;
	while (start < ticks) {
		CheckInput();
		start++;
		swiWaitForVBlank();
	}
}

void POSCursor(uint8_t X, uint8_t Y) {
	iprintf("\x1b[%d;%dH", Y, X);
}

static void RenderBorders(bool DELAY) {
	for (size_t Y = VER_OFFSET; Y <= ROWS; Y++) {
  
		for (size_t X = HOR_OFFSET; X <= COLS; X++) {
  
			if ( ( (X == HOR_OFFSET || X == COLS) && (Y >= VER_OFFSET && Y <= ROWS) )|| (Y == VER_OFFSET || Y == ROWS)) {
  				POSCursor(X, Y);
				if (DELAY) {
  					sleep(10);
				}
		  		iprintf("#");
  			}
		}
	}
}

static void RenderSnake() {
	POSCursor(SnakePOSbuffer[SnakeLength][0], SnakePOSbuffer[SnakeLength][1]);
	if (SnakePOSbuffer[SnakeLength][0] != 0 && SnakePOSbuffer[SnakeLength][1] != 0) {
		iprintf(" ");
	}	
	POSCursor(SnakeX, SnakeY);
	iprintf("#");	
	SnakePOSbuffer[0][0] = SnakeX;
	SnakePOSbuffer[0][1] = SnakeY;
	for (size_t i = SnakeLength; i > 0; i--) {
		SnakePOSbuffer[i][0] = SnakePOSbuffer[i - 1][0];
		SnakePOSbuffer[i][1] = SnakePOSbuffer[i - 1][1];
	}

}

static void GameOver() {
	iprintf("\x1b[2J");
	POSCursor(10, 10);
	iprintf("Game Over!");
	POSCursor(8, 12);
	iprintf("Your Score : %d", Score);
	POSCursor(5, 14);
	iprintf("Press Start to exit or");
	POSCursor(4, 15);
	iprintf("A to restart the game...");

	while (1) {
		scanKeys();
		int pressed = keysDown();
	
		if (pressed & KEY_START) {
			exit(0);
		} else if (pressed & KEY_A) {
			iprintf("\x1b[2J");
			RenderBorders(true);
			return;
		}
	}

}

static void GenerateBall() {
	for (size_t i = 1; i <= SnakeLength; i++) {
		while (BallX < HOR_OFFSET + 1 || BallX > COLS || BallY < VER_OFFSET + 1 || BallY > ROWS || (BallX == SnakePOSbuffer[i][0] && BallY == SnakePOSbuffer[i][1]) || (BallX == ANSBallX && BallY == ANSBallY)) {
			BallX = HOR_OFFSET + 1 + rand() % (COLS - HOR_OFFSET - 1);
			BallY = VER_OFFSET + 1 + rand() % (ROWS - VER_OFFSET - 1);
		}
	}
	BallEaten = false;
	ANSBallX = BallX;
	ANSBallY = BallY;
	POSCursor(BallX, BallY);
	iprintf("O");
}

static void Loose() {
	iprintf("\x1b[2J");
	sleep(1000);
	RenderBorders(false);
	GenBall = true;
	Start = false;
	for (size_t i = 1; i < 254; i++) {
		SnakePOSbuffer[i][0] = 0;
		SnakePOSbuffer[i][1] = 0;
	}
	SnakeX = (COLS/2) + 4;
	SnakeY = ROWS/2;
	VSnakeX = 0;
	VSnakeY = 0;
	SnakeLength = 2;
	if (Lifes > 0) {
		Lifes--;
	} else {
		GameOver();
		Lifes = 3;
		Score = 0;
	}
}

static void ManageSnakePos() {
	CheckInput();
	if (SnakeX < HOR_OFFSET + 1 || SnakeX > COLS - 1 || SnakeY < VER_OFFSET + 1 || SnakeY > ROWS - 1) {
		Loose();
	}
	if (SnakeLength > 4) {
		for (size_t i = 4; i < SnakeLength + 1; i++) {
			if (SnakeX == SnakePOSbuffer[i][0] && SnakeY == SnakePOSbuffer[i][1]) {
				Loose();
			}
		}
	}
		
	if (SnakeX == BallX && SnakeY == BallY && !BallEaten) {
		Score++;
		SnakeLength++;
		GenBall = true;
	}
	SnakeX = SnakeX + VSnakeX;
	SnakeY = SnakeY + VSnakeY;
}

static void PrintGameStats() {
	consoleSelect(&bottomScreen);
	POSCursor(0, 0);
	iprintf(" Score : %d \n", Score);
	iprintf(" Lifes : %d ", Lifes);
	consoleSelect(&topScreen);
}

static void RunGame() {
	sleep(500);
	PrintGameStats();
	if (counter < 4) {
		counter++;
	} else {
		counter = 0;
	}
	if (counter == 3 && GenBall) {
		GenerateBall();
		GenBall = false;
	}
	ManageSnakePos();
	RenderSnake();
	PressedButton = false;
	swiWaitForVBlank();
}

//---------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
//---------------------------------------------------------------------------------

	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);

	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	consoleInit(&topScreen, 3,BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
	consoleInit(&bottomScreen, 3,BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);

	consoleSelect(&topScreen);
	iprintf("\x1b[2J");
	iprintf("Snake DS\nWritten By Abdelali221\nGithub : \nhttps://github.com/abdelali221/\n");
	iprintf("\nPress A to start...");

	while(!Resume) {

		scanKeys();
		int pressed = keysDown();
		if ( pressed & KEY_A ) {
			Resume = true;
		}
		swiWaitForVBlank();
	}

	iprintf("\x1b[2J");
	RenderBorders(true);

	while (1) {
		RunGame();
	}

	return 0;

}
