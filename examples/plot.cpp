#include <wrappy/wrappy.h>
#include <algorithm>

int main() {
	std::vector<double> x {1.0, 2.0, 3.0, 4.0}, y {1.5, 1.0, 1.3, 2.0};
	std::vector<wrappy::PythonObject> pyx, pyy;

	std::transform(x.begin(), x.end(), std::back_inserter(pyx),
            [](double d) { return wrappy::construct(d); });
	std::transform(y.begin(), y.end(), std::back_inserter(pyy),
            [](double d) { return wrappy::construct(d); });

	wrappy::call("matplotlib.pyplot.plot", pyx, pyy);
	wrappy::call("matplotlib.pyplot.show");
}
