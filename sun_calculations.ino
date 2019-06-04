#include <math.h>

double J1970 = 2440588;
double J2000 = 2451545;
double deg2rad = PI / 180;
double M0 = 357.5291 * deg2rad;
double M1 = 0.98560028 * deg2rad;
double J0 = 0.0009;
double J1 = 0.0053;
double J2 = -0.0069;
double C1 = 1.9148 * deg2rad;
double C2 = 0.0200 * deg2rad;
double C3 = 0.0003 * deg2rad;
double P = 102.9372 * deg2rad;
double e = 23.45 * deg2rad;
double h0 = -0.83 * deg2rad;
double msInDay = 1000 * 60 * 60 * 24;

double getJulianFromUnix(long unixSecs)
{
	return (unixSecs / 86400.0) + 2440587.5;
}

double getMillisecondsFromJulianDate(double j)
{
	return (j + 0.5 - J1970) * msInDay;
}

double getJulianCycle(double J, double lw)
{
	return round(J - J2000 - J0 - lw / (2 * PI));
}

double getApproxSolarTransit(double Ht, double lw, double n)
{
	return J2000 + J0 + (Ht + lw) / (2 * PI) + n;
}

double getSolarMeanAnomaly(double Js)
{
	return M0 + M1 * (Js - J2000);
}

double getEquationOfCenter(double M)
{
	return C1 * sin(M) + C2 * sin(2 * M) + C3 * sin(3 * M);
}

double getEclipticLongitude(double M, double C)
{
	return M + P + C + PI;
}

double getSolarTransit(double Js, double M, double Lsun)
{
	return Js + (J1 * sin(M)) + (J2 * sin(2 * Lsun));
}

double getSunDeclination(double Lsun)
{
	return asin(sin(Lsun) * sin(e));
}

double getHourAngle(double h, double phi, double d)
{
	return acos((sin(h) - sin(phi) * sin(d)) /
		(cos(phi) * cos(d)));
}

double getSunsetJulianDate(double w0, double M, double Lsun, double lw, double n)
{
	return getSolarTransit(getApproxSolarTransit(w0, lw, n), M, Lsun);
}

double getSunriseJulianDate(double Jtransit, double Jset)
{
	return Jtransit - (Jset - Jtransit);
}

double getCurrentDayLength(long epochTime, double lat, double lng)
{
	double lw = -lng * deg2rad;
	double phi = lat * deg2rad;
	double J = getJulianFromUnix(epochTime);

	double n = getJulianCycle(J, lw);
	double Js = getApproxSolarTransit(0, lw, n);
	double M = getSolarMeanAnomaly(Js);
	double C = getEquationOfCenter(M);
	double Lsun = getEclipticLongitude(M, C);
	double d = getSunDeclination(Lsun);
	double Jtransit = getSolarTransit(Js, M, Lsun);
	double w0 = getHourAngle(h0, phi, d);

	double Jset = getSunsetJulianDate(w0, M, Lsun, lw, n);
	double Jrise = getSunriseJulianDate(Jtransit, Jset);

	double dayLengthSeconds = (getMillisecondsFromJulianDate(Jset) - getMillisecondsFromJulianDate(Jrise)) / 1000;

	Serial.print("Calculated day length (seconds): ");
	Serial.println(dayLengthSeconds);

	return dayLengthSeconds;
}
