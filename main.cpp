#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include "GraphicsUI.h"
#include "AssetManager.h"
#include "GraphicsRender.h"
#include <ctime>

class GameState {
public:
	enum State {
		Menu = 0,
		Play = 1,
		HighScore = 2,
		Quit = 3
	} state;

	bool isStateChanged;
	GameState() {
		isStateChanged = false;
		LoadAssets();
	}

	virtual void Logic() = 0;
	virtual void ManageEvent(sf::Event, sf::Vector2f) {}
	virtual void Render(sf::RenderWindow&) = 0;

	void LoadAssets() {
		AssetHolder::Get().AddSoundBuffer("beep1", "files/sounds/beep1.wav");
		AssetHolder::Get().AddSoundBuffer("beep2", "files/sounds/beep2.wav");
		AssetHolder::Get().AddSoundBuffer("beep3", "files/sounds/beep3.wav");
		AssetHolder::Get().AddSoundBuffer("beep4", "files/sounds/beep4.wav");

		AssetHolder::Get().AddFont("sansationBold", "files/fonts/Sansation_Bold.ttf");

		AssetHolder::Get().AddTexture("gameTitle", "files/images/gameTitle.png");
	}
};

class MenuState : public GameState {
private:
	std::vector<Button> buttons;
	sf::Vector2f buttonSize;
	sf::Sprite gameTitle;

	const std::string buttonNames[2] = { "Play", "Quit" };
public:
	MenuState() {

		LoadAssets();

		buttonSize = { 200.0f, 50.0f };

		for (int i = 0; i < 2; i++) {
			buttons.push_back(Button());
			buttons[i].Initialize({ 142.25f, 300.0f + (i * (buttonSize.y + 10.0f)) }, buttonSize);
			
			sf::Color randColor = sf::Color(rand() % 100, rand() % 100, rand() % 100);

			buttons[i].SetColors(sf::Color(randColor.r + 100, randColor.g + 100, randColor.b + 100), 
				sf::Color(randColor.r + 50, randColor.g + 50, randColor.b + 50), randColor);

			buttons[i].SetOutline(-5.0f, sf::Color(randColor.r + 10, randColor.g + 10, randColor.b + 10));

			buttons[i].ResetColor();
		}
	
		gameTitle.setTexture(AssetHolder::Get().GetTexture("gameTitle"));
		gameTitle.setPosition({ 8.0f, 10.0f });
	}

	void Logic() override {}

	void ManageEvent(sf::Event e, sf::Vector2f mousePos) override {
		for (auto& button : buttons) {
			button.Logic(e, mousePos);
		}

		switch (e.type) {
		case sf::Event::MouseButtonPressed:
			switch (e.key.code) {
			case sf::Mouse::Left:
				for (std::size_t i = 0; i < buttons.size(); i++) {
					if (buttons[i].IsPositionInBounds(mousePos)) {
						isStateChanged = true;
						switch (i) {
						case 0:
							state = Play;
							break;
						case 1:
							state = Quit;
							break;
						}
					}
				}
				break;
			}
			break;
		}
	}

	void Render(sf::RenderWindow& window) override {
		int index = 0;
		for (auto& button : buttons) {
			button.Render(window);
			RenderText(window, AssetHolder::Get().GetFont("sansationBold"), button.GetPosition().x + buttonSize.x / 2.0f - 30.0f, button.GetPosition().y, buttonNames[index]);
			index++;
		}
	
		window.draw(gameTitle);
	}
};

class PlayState : public GameState {
private:
	sf::RectangleShape scoreBox;
	std::vector<Button> buttons;
	int index, nSequences, score;
	bool isColorsRendered;
	Sound sound;
	
	sf::Clock clock;
	float dt, delay;

	struct VectorIntPair {
		std::vector<int> vectorInput;
		std::vector<int> vectorOutput;

		void ClearVectors() {
			vectorInput.clear();
			vectorOutput.clear();
		}

		void GenerateInputSequence(int n, uint32_t size) {
			ClearVectors();
			for (int i = 0; i < n; i++) {
				vectorInput.push_back(rand() % size);
			}
		}

		bool IsVectorsEqual() {
			return vectorInput == vectorOutput;
		}
	};

	VectorIntPair buttonSequences;

	void RandomizeButtonColors() {
		for (int i = 0; i < (int)buttons.size(); i++) {
			sf::Color randColor = sf::Color(rand() % 100, rand() % 100, rand() % 100);

			buttons[i].SetColors(sf::Color(randColor.r + 100, randColor.g + 100, randColor.b + 100),
				sf::Color(randColor.r + 50, randColor.g + 50, randColor.b + 50), randColor);

			buttons[i].ResetColor();
		}
	}
public:
	PlayState() {
		LoadAssets();

		index = score = 0;
		dt = 0.0f;
		delay = 1.0f;
		isColorsRendered = false; //Is Color Sequence shown
		nSequences = 1;

		scoreBox.setSize({ 485.0f, 32.0f });
		scoreBox.setPosition({ 0.0f, 0.0f });
		scoreBox.setFillColor(sf::Color(rand() % 256, rand() % 256, rand() % 256));

		int pos = 0;
		for (int i = 0; i < 4; i++) {
			buttons.push_back(Button());
			buttons[i].Initialize({ (i % 2) * 235.0f + 15.0f, pos * 235.0f + 45.0f }, { 220.0f, 220.0f });

			buttons[i].ResetColor();

			if (i == 1) pos++;
		}
		RandomizeButtonColors();

		buttonSequences.GenerateInputSequence(nSequences, buttons.size());
	}

	void ResetButtonColors() {
		for (auto& button : buttons) {
			button.SetFillColor(button.GetColors()[Button::ButtonState::MouseRelease]);
		}
	}

	void Logic() override {

		float frameDt = (float)clock.getElapsedTime().asSeconds();
		clock.restart();

		if (!isColorsRendered) {
			if (buttonSequences.vectorInput.size() > 0) {

				dt += frameDt;

				ResetButtonColors();
				int n = buttonSequences.vectorInput[index];
				if (((int)dt % 2) != 0) buttons[n].SetFillColor(buttons[n].GetColors()[Button::ButtonState::MousePressed]);

				if (dt > 2 * delay) {
					index++;
					if (index > (nSequences - 1)) {
						index = 0;
						isColorsRendered = true;
						ResetButtonColors();
					}
					dt = 0.0f;
				}
			}
		}
		else {
			if (buttonSequences.IsVectorsEqual()) {
				dt += frameDt;

				ResetButtonColors();
				if (((int)dt % 2) != 0) {
					for (auto& button : buttons) {
						button.SetFillColor(button.GetColors()[Button::ButtonState::MousePressed]);
					}
				}

				if (dt > 2 * delay) {
					isColorsRendered = false;
					dt = 0.0f;
					nSequences++;
					RandomizeButtonColors();
					buttonSequences.GenerateInputSequence(nSequences, buttons.size());
					score++;
				}
			}
			else {
				if (buttonSequences.vectorOutput.size() > buttonSequences.vectorInput.size()) {

					dt += frameDt;
					
					ResetButtonColors();
					if (((int)dt % 2) != 0) {
						for (auto& button : buttons) {
							button.SetFillColor(sf::Color::Red);
						}
					}

					if (dt > 2 * delay) {
						isColorsRendered = false;
						dt = 0.0f;
						RandomizeButtonColors();
						buttonSequences.GenerateInputSequence(nSequences, buttons.size());
					}
				}
			}
		}
	}

	void ManageEvent(sf::Event e, sf::Vector2f mousePos) override {
		for (auto& button : buttons) {
			button.Logic(e, mousePos);
		}

		switch (e.type) {
		case sf::Event::MouseButtonPressed:
			switch (e.key.code) {
			case sf::Mouse::Left:
				if (isColorsRendered) {
					for (std::size_t i = 0; i < buttons.size(); i++) {
						if (buttons[i].IsPositionInBounds(mousePos)) {

							switch (i) {
							case 0:
								sound.setBuffer(AssetHolder::Get().GetSoundBuffer("beep1"));
								break;
							case 1:
								sound.setBuffer(AssetHolder::Get().GetSoundBuffer("beep2"));
								break;
							case 2:
								sound.setBuffer(AssetHolder::Get().GetSoundBuffer("beep3"));
								break;
							case 3:
								sound.setBuffer(AssetHolder::Get().GetSoundBuffer("beep4"));
								break;
							}

							sound.play();

							buttonSequences.vectorOutput.push_back(i);
							break;
						}
					}
				}
				break;
			}
			break;
		case sf::Event::KeyPressed:
			switch (e.key.code) {
			case sf::Keyboard::Escape:
				state = Menu;
				isStateChanged = true;
				break;
			}
			break;
		}
	}

	void Render(sf::RenderWindow& window) override {
		for (auto& button : buttons) {
			button.Render(window);
		}

		window.draw(scoreBox);
		DrawTextWithValue(window, AssetHolder::Get().GetFont("sansationBold"), 0.0f, 0.0f, "Score : ", score);
	}
}; 

class Game {
private:
	sf::RenderWindow Window;
	sf::Vector2u windowSize;

	std::unique_ptr<GameState> gameState;
public:
	Game(const sf::Vector2u size, const sf::String& title)
		: Window({ size.x, size.y }, title),
		  windowSize(size) {
		Window.setFramerateLimit(60);

		gameState = std::make_unique<MenuState>();
	}

	void Run() {
		Logic();
	}

	void Logic() {
		while (Window.isOpen()) {
			sf::Event event;

			sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(Window);

			if (gameState->isStateChanged) {
				if (gameState->state == GameState::State::Play) gameState = std::make_unique<PlayState>();
				else if (gameState->state == GameState::State::Menu) gameState = std::make_unique<MenuState>();
				else if (gameState->state == GameState::State::Quit) Window.close();
				gameState->isStateChanged = false;
			}

			while (Window.pollEvent(event)) {
				switch (event.type) {
				case sf::Event::Closed:
					Window.close();
					break;
				}
			
				gameState->ManageEvent(event, mousePos);
			}

			gameState->Logic();

			Window.clear();
			gameState->Render(Window);
			Window.display();
		}
	}
};

int main() {

	srand((unsigned)time(0));

	Game game({ 485, 515 }, "Game");
	game.Run();

	return 0;
}