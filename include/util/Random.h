/**
 * @file Random.h
 * @brief ���i���ȗ������ȒP�Ɏg�����߂̃��[�e�B���e�B
 * @author �R���z
 * @date 2025
 * @version 1.0
 *
 * @details
 * C�� rand()/srand() �̑���ɁAC++�� <random> ���g�������i���ȗ���������񋟂��܂��B
 * �g�����̓V���v���ŁA`Random::Float(min, max)` �� `Random::Int(min, max)` ���ĂԂ����ł��B
 * �K�v�ɉ����� `Random::Seed(seed)` �ŃV�[�h�Œ���\�ł��i���v���C�Č��Ȃǂɕ֗��j�B
 *
 * - �X���b�h���[�J���� `std::mt19937` ������Ŏg�p
 * - �ǉ��R�X�g�Ȃ��ň��S�ɕ����ӏ����痘�p�\
 * - C++14�Ή��i�w�b�_�I�����[�j
 */
#pragma once

#include <random>
#include <cstdint>
#include <limits>
#include <chrono>
#include <cmath>
#include <DirectXMath.h>

namespace util {

class Random {
public:
    // �C�ӂ̌Œ�V�[�h�ŏ������i���݂̃X���b�h�̃G���W�����ăV�[�h�j
    static void Seed(uint32_t seed) {
        Engine() .seed(seed);
    }

    // ���ݎ����ŃV�[�h�i�ȈՁj
    static void SeedTime() {
        uint32_t seed = static_cast<uint32_t>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count() & 0xFFFFFFFFULL);
        Seed(seed);
    }

    // [min, max] �̎�����l�����i��ԁj
    static float Float(float minInclusive, float maxInclusive) {
        if (minInclusive > maxInclusive) std::swap(minInclusive, maxInclusive);
        std::uniform_real_distribution<float> dist(minInclusive, nextUp(maxInclusive));
        return dist(Engine());
    }

    // [min, max] �̐�����l�����i��ԁj
    static int Int(int minInclusive, int maxInclusive) {
        if (minInclusive > maxInclusive) std::swap(minInclusive, maxInclusive);
        std::uniform_int_distribution<int> dist(minInclusive, maxInclusive);
        return dist(Engine());
    }

    // true ��Ԃ��m�� p�i0..1�j
    static bool Bool(float p = 0.5f) {
        if (p <= 0.0f) return false;
        if (p >= 1.0f) return true;
        std::bernoulli_distribution dist(p);
        return dist(Engine());
    }

    // ���K���z N(mean, stddev)
    static float Normal(float mean = 0.0f, float stddev = 1.0f) {
        if (stddev <= 0.0f) return mean;
        std::normal_distribution<float> dist(mean, stddev);
        return dist(Engine());
    }

    // [0,1] �̖���߃J���[�i0.33�`1.0�j
    static DirectX::XMFLOAT3 ColorBright() {
        return DirectX::XMFLOAT3{ Float(0.33f, 1.0f), Float(0.33f, 1.0f), Float(0.33f, 1.0f) };
    }

    // [min,max] �̃J���[
    static DirectX::XMFLOAT3 Color(float minInclusive = 0.0f, float maxInclusive = 1.0f) {
        return DirectX::XMFLOAT3{ Float(minInclusive, maxInclusive),
                                   Float(minInclusive, maxInclusive),
                                   Float(minInclusive, maxInclusive) };
    }

    // ��l�ȒP�ʃx�N�g���i�ߎ��j
    static DirectX::XMFLOAT3 UnitVec3() {
        // ���ʈ�l���z�i�p�x���琶���j
        const float z = Float(-1.0f, 1.0f);
        const float t = Float(0.0f, 6.28318530718f); // 2��
        const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
        return DirectX::XMFLOAT3{ r * std::cos(t), r * std::sin(t), z };
    }

private:
    // �X���b�h���[�J���ȃG���W���i�ŏ��̎g�p���Ƀ����_���f�o�C�X�ŏ������j
    static std::mt19937& Engine() {
        thread_local std::mt19937 rng(seedFromDevice());
        return rng;
    }

    static uint32_t seedFromDevice() {
        std::random_device rd;
        // �ꕔ���� rd() ����i���ȏꍇ�����邽�߁A�����񍬍�
        uint32_t s = rd();
        s ^= (rd() << 1);
        s ^= (rd() << 2);
        return s;
    }

    // ������̗אڕ��������i��Ԃ��������邽�߂̔����I�t�Z�b�g�j
    static float nextUp(float x) {
        if (std::isinf(x) && x > 0) return x;
        if (std::isnan(x)) return x;
        return std::nextafter(x, std::numeric_limits<float>::infinity());
    }
};

} // namespace util
