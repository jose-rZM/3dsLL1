#include <3ds.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include "ll1/grammar.hpp"
#include "ll1/ll1_parser.hpp"

std::unordered_map<std::string, std::vector<std::vector<std::string>>> grammar;

void ingestProduction(const std::string& line) {
    size_t pos = line.find(":");
    if (pos == std::string::npos) {
        printf("Invalid production\n");
        return;
    }

    std::string lhs = line.substr(0, pos);
    std::string rhs = line.substr(pos + 1);

    lhs.erase(lhs.find_last_not_of(" \n\r\t") + 1);
    lhs.erase(0, lhs.find_first_not_of(" \n\r\t"));
    
    std::istringstream iss(rhs);
    std::vector<std::string> prod;
    std::string smb;
    while (iss >> smb) {
        prod.push_back(smb);
    }

    grammar[lhs].push_back(prod);
    printf("Ingested! %s\n", line.c_str());
}

void ingestGrammar(const std::string& text) {
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        ingestProduction(line);
    }
}

std::string read() {
    SwkbdState swkbd;
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, -1);
    swkbdSetFeatures(&swkbd, SWKBD_MULTILINE);
    swkbdSetHintText(&swkbd, "Format A: a A");
    char input[256];
    SwkbdButton button = swkbdInputText(&swkbd, input, sizeof(input));

    if (button == SWKBD_BUTTON_CONFIRM) {
        return std::string(input);
    } else {
        return "";
    }
}


void printPaginated(const std::string& text) {
    constexpr int linesPerPage = 20;
    std::vector<std::string> lines;
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }

    int totalLines = lines.size();
    int offset = 0;
    bool needsRedraw = true;

    while (true) {
        if (needsRedraw) {
            consoleClear();
            for (int i = 0; i < linesPerPage && (offset + i) < totalLines; ++i) {
                printf("%s\n", lines[offset + i].c_str());
            }
    
            printf("\nUP or DOWN to scroll, SELECT to stop.\n");
            needsRedraw = false;
            gfxFlushBuffers();
            gfxSwapBuffers();
        }

        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_SELECT) break;
        if (kDown & KEY_DOWN) {
            if (offset + linesPerPage < totalLines) {
                offset += linesPerPage;
                needsRedraw = true;
            }
        }
        if (kDown & KEY_UP) {
            if (offset - linesPerPage >= 0) {
                offset -= linesPerPage;
                needsRedraw = true;
            } else if (offset != 0) {
                offset = 0;
                needsRedraw = true;
            } 
        }

        gspWaitForVBlank();
    }
}

void clean(PrintConsole* top, PrintConsole* bottom) {
    consoleSelect(top);
    consoleClear();
    consoleSelect(bottom);
    consoleClear();
    consoleSelect(top);
}

void help() {
    printf("LLBrew\n");
    printf("Write a grammar with A.\n");
    printf("Press B to process.\n");
    printf("Y to reset.\n");
    printf("START to exit.\n");
    printf("The axiom must be unique and with the following format: NT: <text> $, where $ is the EOL character.\n");
}

int main(int argc, char **argv)
{
	gfxInitDefault();
    PrintConsole topScreen, bottomScreen;
    consoleInit(GFX_TOP, &topScreen);
    consoleInit(GFX_BOTTOM, &bottomScreen);
    consoleSelect(&topScreen);
	clean(&topScreen, &bottomScreen);
    help();
	while (aptMainLoop())
	{
        std::string line;
		hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) break;

        if (kDown & KEY_Y) {
            line.clear();
            clean(&topScreen, &bottomScreen);
            help();
            grammar.clear();
            continue;
        }

        if (kDown & KEY_A) {
            line = read();
            ingestGrammar(line);
        }

        if (kDown & KEY_B) {
            if (grammar.empty()) {
                printf("Grammar is empty.\n");
                continue;
            }
            consoleClear();
            Grammar gr(grammar);
            consoleSelect(&bottomScreen);
            gr.Debug();
            gfxFlushBuffers();
            gfxSwapBuffers();
            consoleSelect(&topScreen);
            LL1Parser ll1(gr);
            bool isll1 = ll1.CreateLL1Table();
            printf("\nIs LL(1)?: %s\n", isll1 ? "Yes" : "No");
            std::string table = ll1.PrintTable();
            printPaginated(table);
            printf("Press Y to restart or START to exit.\n"); 
        }

		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}
