#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <sstream>
#include <memory>
#include <iostream>
#include <fftw3.h>
#define _USE_MATH_DEFINES
#include <math.h>


int main() {
	sf::RenderWindow window; // RenderWindow instead of Window
	window.create(sf::VideoMode({ 1280, 720 }), "Music Visualizer :)");

	sf::Font font;
	if (!font.openFromFile("NotoSans-VariableFont_wdth,wght.ttf"))
	{
		return -1;
	}

	sf::Texture texture;
	if (!texture.loadFromFile("bg.png"))
	{
		return -1;
	}
	sf::Sprite sprite(texture); // background image
	
	sf::SoundBuffer buffer;
	//music.flac
	//music_.flac
	//welcome.flac
	if (!buffer.loadFromFile("music_.flac")) 
	{
		return -1;
	}

	sf::Sound sound(buffer);
	sound.play();
	sound.setVolume(50);
	

	// Text Settings
	sf::Text text(font);
	//text.setString("Mitsukiyo - Melodies of Dawn (Piano Ver.)");
	text.setString("OSTER project - Alice in Musicland (Ending)");
	//text.setString("Mitsukiyo - Welcome School");
	text.setCharacterSize(56);
	text.setPosition({58.f, 3.f});
	text.setFillColor(sf::Color::White);
	text.setOutlineColor(sf::Color::Black);
	text.setOutlineThickness(2.f);
	text.setStyle(sf::Text::Bold);
	
	//fftw plan
	auto sampleSize = 4096;
	std::vector<float> spectrum(sampleSize / 2);
	const int16_t* samples = buffer.getSamples();

	size_t sampleCount = buffer.getSampleCount();
	size_t channels = buffer.getChannelCount();
	
	// hamming window
	std::vector<double> hammingWindow(sampleSize);
	for (int i = 0; i < sampleSize; ++i) {
		hammingWindow[i] = 0.54 - 0.46 * std::cos(2.0 * M_PI * i / (sampleSize - 1));
	}

	fftw_complex* in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * sampleSize);
	fftw_complex* out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * sampleSize);

	fftw_plan plan = fftw_plan_dft_1d(sampleSize, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

	// Main Window Loop
	while (window.isOpen()) {
		while (const std::optional event = window.pollEvent()) {
			if (event->is<sf::Event::Closed>())
				window.close();
		}
		// continously update the offset to line up the fft with the music
		float soundTime = sound.getPlayingOffset().asSeconds();
		size_t currentSample = static_cast<size_t>(soundTime * buffer.getSampleRate());

		sf::Text time(font);
		std::stringstream ss;
		ss.precision(2);

		int minutes = static_cast<int>(soundTime) / 60;
		int seconds = static_cast<int>(soundTime) % 60;
		ss << minutes << ":" << (seconds < 10 ? "0" : "") << seconds;
		time.setString(ss.str());
		time.setCharacterSize(36);
		time.setPosition({ 30.f, 650.f });
		time.setFillColor(sf::Color::White);
		time.setOutlineColor(sf::Color::Black);
		time.setOutlineThickness(2.f);
		time.setStyle(sf::Text::Bold);


		for (int i = 0; i < sampleSize; i++) {
			size_t index = (currentSample + i) % (sampleCount / channels);
			double windowedSample = (samples[index * channels] / 32768.0) * hammingWindow[i];

			in[i][0] = windowedSample; // Real part
			in[i][1] = 0.0;            // Imaginary part
		}

		fftw_execute(plan);

		for (int i{}; i < sampleSize / 2; i++) {
			spectrum[i] = std::sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]); // sqrt of re^2 + im^2
		}

		currentSample += sampleSize;


		window.setFramerateLimit(60);
		window.clear(sf::Color::Black);
		window.draw(sprite);
		window.draw(text);
		window.draw(time);
		
		
		// Visualizer Drawing
		const int numBars = 150;
		const float barWidth = 5.f;
		const float barSpacing = 3.f;

		float totalWidth = numBars * barWidth + (numBars - 1) * barSpacing;
		float startX = (1280 - totalWidth) / 2.f;  // Center horizontally
		float baselineY = 720.f - 100.f;           // Put spectrum near bottom

		for (int i = 0; i < numBars; ++i) {
			float magnitude = 0.8 * spectrum[i]; // scale your FFT magnitude
			sf::RectangleShape bar;

			bar.setSize(sf::Vector2f(barWidth, -magnitude)); // Negative height to grow upward
			bar.setPosition({ startX + i * (barWidth + barSpacing), baselineY });
			bar.setFillColor(sf::Color::Blue);

			window.draw(bar);
		}


		window.display();
		
		bool zKeyPressed = false; 

		
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z)){
			if (sound.getStatus() != sf::Sound::Status::Playing) { // only active when it's status is other than playing
				sound.play();
			}
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X)) {
			if (sound.getStatus() == sf::Sound::Status::Playing) {
				sound.pause();
			}
		}


		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)) {
			sound.stop();
		}
		
	}
	fftw_destroy_plan(plan);
	fftw_free(in);
	fftw_free(out);

	return EXIT_SUCCESS;
}
