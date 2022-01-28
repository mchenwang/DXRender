#include "stdafx.h"
#include "DXWindow.h"
// #include "Win32App.h"
#include "Application.h"

#include <iostream>
using namespace std;

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    // Win32Application app(window, hInstance);
    Application::CreateInstance(hInstance);

    auto app = Application::GetInstance();

    // auto model = make_shared<Model>(L"bun_zipper.ply", ModelType::PLY);
    auto model = make_shared<Model>(L"african_head.obj", ModelType::OBJ);
    app->SetModel(model);

    auto window = make_shared<DXWindow>(L"Learn DX12");
    auto hWnd = app->CreateWindow(hInstance, window.get());
    window->Init(hWnd);
    return app->Run(window);
}