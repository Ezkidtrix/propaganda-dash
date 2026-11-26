#include <Geode/Geode.hpp>
#include <Geode/binding/FLAlertLayer.hpp>
#include <Geode/modify/CreatorLayer.hpp>

#include "Geode/ui/Layout.hpp"
#include "Geode/cocos/cocoa/CCObject.h"
#include "cpp-llamalib.h"

using namespace geode::prelude;

class $modify(MyCreatorLayer, CreatorLayer) {
  struct Fields {
    CCNode* m_menu;
    std::unique_ptr<llamalib::Llama> m_llm;
  };

  bool init() {
    if (!CreatorLayer::init()) return false;

    m_fields->m_menu = this->getChildByID("bottom-left-menu");
    if (!m_fields->m_menu) return true;

    this->setupButton();
    m_fields->m_menu->updateLayout();
    
    this->loadLLM();
    return true;
  }

  void setupButton() {
    auto spr = ButtonSprite::create("AI");
    auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MyCreatorLayer::askLLM));

    btn->setID("respawn-btn"_spr);
    auto options = geode::AxisLayoutOptions::create()->setScaleLimits(0.6f, 1.0f);

    btn->setLayoutOptions(options);
    m_fields->m_menu->addChild(btn);
  }

  void loadLLM() {
    auto path = Mod::get()->getResourcesDir() / "SmolLM-135M-Instruct-Q3_K_M.gguf";
    static geode::async::TaskHolder<> loader;

    loader.spawn([this, path]()->arc::Future<> {
      llamalib::Options options = { .n_gpu_layers = 0, .n_ctx = 512 };
      m_fields->m_llm = std::make_unique<llamalib::Llama>(path.string(), options);

      co_return;
    }, [this]() {
      log::info("LLM Loaded: {}", m_fields->m_llm ? "OK" : "Failed!");
    });
  }

  void askLLM(CCObject*) {
    if (!m_fields->m_llm) return;
    log::info("Attempting LLM Response!");

    llamalib::GenerateOptions options = { 
      .max_tokens = 40, 
      .grammar = R"(
        root ::= word ( " " word ){0,15}
        word ::= [^ \n]+
      )",
      .temperature = 0.4f 
    };
    std::string text = m_fields->m_llm->generate(
      "Tell me a random fun fact within 16 words or less.\nAnswer: ", options
    );

    if (auto p = text.find(": "); p != std::string::npos) text.erase(0, p + 2);
    FLAlertLayer::create("AI TEST", text, "Ok")->show();
  }
};