from scipy import *
from scipy import integrate
from pylab import *

def myfunc(x):
	return sqrt(1-x*x)

def simpson(f, a ,b, n):
	"""f=name of function, a=initial value, b=end value, n=number of double intervals of size 2h"""

	n *= 2
	h = (b - a) / n;
	S = f(a)

	for i in range(1, n, 2):
		x = a + h * i
		S += 4 * f(x)

	for i in range(2, n-1, 2):
		x = a + h * i
		S += 2 * f(x)
	
	S += f(b)
	F = h * S / 3
 
	return F

def trapezoidal(f, a ,b, n):
	"""f=name of function, a=initial value, b=end value, n=number intervals """

	h = (b - a) / n;
	S = f(a)

	for i in range(1, n, 1):
		x = a + h * i
		S += 2 * f(x)
	
	S += f(b)
	F = h * S / 2
 
	return F

res,err = integrate.quad(myfunc,0,1)

err1 = zeros(20)
err2 = zeros(20)
i = 0
for j in range(5,25,1):
	res1 = trapezoidal(myfunc,0,1.0,j*2)
	res2 = simpson(myfunc,0,1.0,j)
	err1[i] = abs(res-res1)
	err2[i] = abs(res-res2)
	i += 1

plot(err1,'b')
plot(err2,'r+')
show()
