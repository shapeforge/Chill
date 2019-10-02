#include <SDL.h>

#include <vector>
#include <map>

class EventManager {
public :
	EventManager* getInstance() {
		if (!instance) {
			instance = new EventManager();
		}
		return instance;
	};

	void followKey(char * key);

	void isKeyPressed(char * key);
	void isKeyHold(char * key);
	void isKeyRealised(char * key);

	void isMouseClicked(int mousebutton = 0);
	void isMouseHold(int mousebutton = 0);
	void isMouseRealised(int mousebutton = 0);

	void update(); //to use every frame


private :
	EventManager() {
		keyState = std::vector<std::tuple<bool, bool, bool>>(3); //the three first place are reserved for the mouse event
	}
	

	static EventManager *instance;
	std::vector<std::tuple<bool,bool,bool>> keyState; //pressed, released, hold
	std::map<SDL_Keycode, int> keylocation;
	
};