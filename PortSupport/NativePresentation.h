#pragma once
#include <memory>
#include <rex/system/interfaces/graphics.h>
namespace rex { class Runtime; }
std::unique_ptr<rex::system::IGraphicsSystem> SRPrepareGraphics();
void SRConnectPresenter(rex::Runtime* runtime);
