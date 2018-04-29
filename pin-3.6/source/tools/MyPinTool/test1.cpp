#include <iostream>
#include <thread>
#include <chrono>

int f (int a, unsigned int b) {
	std::this_thread::sleep_for (std::chrono::seconds(b));
	return a + b;
}


int main() {
	int tot = 0;
	for (int i = 0; i < 5; i++) 
		tot += f(10, i);
}
		
