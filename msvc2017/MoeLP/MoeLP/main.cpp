#include "../../../src/Base/Console.hpp"
#include "../../../src/Base/Traits.hpp"
#include <cstdlib>
#include <iostream>
#include <type_traits>
using namespace MoeLP;
using namespace std;
struct Test
{
	Test(int t)
	{
		a = t;
	}

	void print()
	{
		Console.writeLine(L"{0}", a);
	}

	int a;
};

int main()
{
	{
		Ptr<int> p = Ptr<int>::create(sizeof(int) * 3);
		p[0] = 1;
		p[1] = 2;
		p[2] = 3;
		Console.writeLine(L"{0} {1} {2}", p[0], p[1], p[2]);
	}
	
	system("pause");
	return 0;
}