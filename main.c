#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define ID_EDIT 101
#define ID_BTN_BUILD 102

// Node structure for Expression Tree
typedef struct Node {
    char value;
    struct Node* left;
    struct Node* right;
    int x; // X coordinate for drawing
    int y; // Y coordinate for drawing
} Node;

Node* root = NULL;
char errorMessage[256] = "";

Node* createNode(char value) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->value = value;
    newNode->left = NULL;
    newNode->right = NULL;
    newNode->x = 0;
    newNode->y = 0;
    return newNode;
}

void freeTree(Node* node) {
    if (node == NULL) return;
    freeTree(node->left);
    freeTree(node->right);
    free(node);
}

// Stack for Node pointers (used in tree building)
Node* nodeStack[100];
int nodeTop = -1;
void pushNode(Node* node) { nodeStack[++nodeTop] = node; }
Node* popNode() { return nodeTop == -1 ? NULL : nodeStack[nodeTop--]; }

// Stack for chars (used in Shunting Yard)
char charStack[100];
int charTop = -1;
void pushChar(char c) { charStack[++charTop] = c; }
char popChar() { return charTop == -1 ? '\0' : charStack[charTop--]; }
char peekChar() { return charTop == -1 ? '\0' : charStack[charTop]; }

int precedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    if (op == '^') return 3;
    return 0;
}

int isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}

// Shunting Yard Algorithm
void infixToPostfix(const char* infix, char* postfix) {
    int i = 0, j = 0;
    charTop = -1; // reset char stack
    while (infix[i] != '\0') {
        if (infix[i] == ' ' || infix[i] == '\t') {
            i++;
            continue;
        }
        if (isalnum(infix[i])) {
            postfix[j++] = infix[i];
        } else if (infix[i] == '(') {
            pushChar(infix[i]);
        } else if (infix[i] == ')') {
            while (charTop != -1 && peekChar() != '(') {
                postfix[j++] = popChar();
            }
            if (charTop != -1 && peekChar() == '(') {
                popChar(); // Pop '('
            } else {
                strcpy(errorMessage, "Mismatched parentheses");
                postfix[0] = '\0';
                return;
            }
        } else if (isOperator(infix[i])) {
            while (charTop != -1 && precedence(peekChar()) >= precedence(infix[i])) {
                postfix[j++] = popChar();
            }
            pushChar(infix[i]);
        } else {
            sprintf(errorMessage, "Invalid character: %c", infix[i]);
            postfix[0] = '\0';
            return;
        }
        i++;
    }
    while (charTop != -1) {
        if (peekChar() == '(') {
            strcpy(errorMessage, "Mismatched parentheses");
            postfix[0] = '\0';
            return;
        }
        postfix[j++] = popChar();
    }
    postfix[j] = '\0';
}

// Build Expression Tree from Postfix
Node* buildTree(const char* postfix) {
    if (postfix[0] == '\0') return NULL;
    nodeTop = -1; // reset node stack
    for (int i = 0; postfix[i] != '\0'; i++) {
        if (isalnum(postfix[i])) {
            pushNode(createNode(postfix[i]));
        } else if (isOperator(postfix[i])) {
            Node* rightNode = popNode();
            Node* leftNode = popNode();
            if (leftNode == NULL || rightNode == NULL) {
                strcpy(errorMessage, "Invalid expression structure");
                return NULL;
            }
            Node* opNode = createNode(postfix[i]);
            opNode->left = leftNode;
            opNode->right = rightNode;
            pushNode(opNode);
        }
    }
    Node* treeRoot = popNode();
    if (nodeTop != -1) { // More than one tree left in stack
        strcpy(errorMessage, "Invalid expression structure");
        return NULL;
    }
    return treeRoot;
}

// GUI AND DRAWING LOGIC

int calculateCoordinates(Node* node, int depth, int xOffset, int* nextX) {
    if (node == NULL) return 0;
    
    // In-order traversal to calculate X
    int hLeft = calculateCoordinates(node->left, depth + 1, xOffset, nextX);
    
    node->x = *nextX;
    node->y = depth * 60 + 80; // 60 px vertical spacing, 80px offset from top
    *nextX += 40; // 40 px horizontal spacing between nodes
    
    int hRight = calculateCoordinates(node->right, depth + 1, xOffset, nextX);
    
    int height = (hLeft > hRight ? hLeft : hRight) + 1;
    return height;
}

void drawTree(HDC hdc, Node* node) {
    if (node == NULL) return;
    
    // Draw lines to children
    if (node->left) {
        MoveToEx(hdc, node->x, node->y, NULL);
        LineTo(hdc, node->left->x, node->left->y);
        drawTree(hdc, node->left);
    }
    if (node->right) {
        MoveToEx(hdc, node->x, node->y, NULL);
        LineTo(hdc, node->right->x, node->right->y);
        drawTree(hdc, node->right);
    }
    
    // Draw current node
    int radius = 15;
    
    // White background for circle
    SelectObject(hdc, GetStockObject(WHITE_BRUSH));
    Ellipse(hdc, node->x - radius, node->y - radius, node->x + radius, node->y + radius);
    
    // Draw text centered
    char str[2] = { node->value, '\0' };
    SetBkMode(hdc, TRANSPARENT);
    
    SIZE textSize;
    GetTextExtentPoint32(hdc, str, 1, &textSize);
    TextOut(hdc, node->x - textSize.cx / 2, node->y - textSize.cy / 2, str, 1);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit;
    static HWND hButton;
    
    switch (msg) {
        case WM_CREATE:
            hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", 
                                   WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 
                                   20, 20, 300, 25, hwnd, (HMENU)ID_EDIT, 
                                   GetModuleHandle(NULL), NULL);
                                   
            hButton = CreateWindow("BUTTON", "Build Tree", 
                                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 
                                   330, 20, 100, 25, hwnd, (HMENU)ID_BTN_BUILD, 
                                   GetModuleHandle(NULL), NULL);
            break;
            
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BTN_BUILD) {
                char infix[256];
                GetWindowText(hEdit, infix, 256);
                
                // Reset State
                strcpy(errorMessage, "");
                freeTree(root);
                root = NULL;
                
                if (strlen(infix) > 0) {
                    char postfix[256] = "";
                    infixToPostfix(infix, postfix);
                    
                    if (strlen(errorMessage) == 0 && strlen(postfix) > 0) {
                        root = buildTree(postfix);
                        if (root) {
                            int nextX = 50; // Starting X coordinate
                            calculateCoordinates(root, 0, 0, &nextX);
                        }
                    }
                }
                // Force window redraw
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Set font
            HFONT hFont = CreateFont(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                     DEFAULT_PITCH | FF_SWISS, "Arial");
            SelectObject(hdc, hFont);
            
            if (strlen(errorMessage) > 0) {
                SetTextColor(hdc, RGB(255, 0, 0));
                TextOut(hdc, 20, 60, errorMessage, strlen(errorMessage));
            } else if (root != NULL) {
                // Set pen for lines
                HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
                SelectObject(hdc, hPen);
                drawTree(hdc, root);
                DeleteObject(hPen);
            }
            
            DeleteObject(hFont);
            EndPaint(hwnd, &ps);
            break;
        }
            
        case WM_DESTROY:
            freeTree(root);
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = "ExpressionTreeClass";
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "ExpressionTreeClass",
        "Expression Tree Builder GUI",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    if(hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while(GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
