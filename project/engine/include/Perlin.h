// http://www.flipcode.com/archives/Perlin_Noise_Class.shtml
#pragma once

class Perlin
{
public:
	Perlin() {}
	Perlin(int octaves, float freq, float amp);

	void Change(int size, int octaves, float freq, float amp);

	float Get(Vec2 pos) const;
	float Get(float x, float y) const
	{
		return Get(Vec2(x, y));
	}

private:

	//float noise1(float arg);
	float Noise2(const Vec2& pos) const;
	//float noise3(float vec[3]);

	int size, size2, octaves;
	float frequency, amplitude;

	vector<int> p;
	vector<Vec2> g2;
	//int p[SAMPLE_SIZE + SAMPLE_SIZE + 2];
	//float g3[SAMPLE_SIZE + SAMPLE_SIZE + 2][3];
	//float g2[SAMPLE_SIZE + SAMPLE_SIZE + 2][2];
	//float g1[SAMPLE_SIZE + SAMPLE_SIZE + 2];
};
