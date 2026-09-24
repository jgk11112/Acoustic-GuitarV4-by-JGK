#include "GuitarEngine.h"
#include <BinaryData.h>
#include <algorithm>
#include <cmath>

float GuitarEngine::coefficientForSeconds(double sr, float seconds)
{
    sr = juce::jmax(8000.0, sr);
    const float safe = juce::jmax(0.004f, seconds);
    return std::exp(std::log(0.001f) / (safe * (float) sr));
}

float GuitarEngine::nextNoise(uint32_t& state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return ((float) (state & 0x00FFFFFFu) / 8388607.5f) - 1.0f;
}

void GuitarEngine::prepare(double newSampleRate)
{
    sampleRate = juce::jmax(8000.0, newSampleRate);
    if (samples.empty())
        loadSamples();

    reverb.setSampleRate(sampleRate);
    reset();
}

void GuitarEngine::reset()
{
    for (auto& v : voices) v = {};
    for (auto& v : transientVoices) v = {};
    for (auto& e : chokeEvents) e = {};
    reverb.reset();
}

void GuitarEngine::loadSamples()
{
    samples.clear();

    juce::WavAudioFormat wav;

    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const juce::String resourceName(BinaryData::namedResourceList[i]);
        const int marker = resourceName.indexOfIgnoreCase("MartinGM2_");
        if (marker < 0)
            continue;

        const int root = resourceName.substring(marker + 10, marker + 13).getIntValue();
        if (root < 20 || root > 110)
            continue;

        int dataSize = 0;
        const char* data = BinaryData::getNamedResource(resourceName.toRawUTF8(), dataSize);
        if (data == nullptr || dataSize <= 44)
            continue;

        auto stream = std::make_unique<juce::MemoryInputStream>(data, (size_t) dataSize, false);
        std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(stream.release(), true));
        if (reader == nullptr || reader->lengthInSamples < 2 || reader->sampleRate <= 0.0)
            continue;

        const auto safeLength = (int) juce::jmin<juce::int64>(reader->lengthInSamples, 60 * 44100);
        if (safeLength < 2)
            continue;

        GuitarSample s;
        s.rootMidi = root;
        s.sourceRate = reader->sampleRate;
        s.audio.setSize(1, safeLength, false, true, true);
        reader->read(&s.audio, 0, safeLength, 0, true, false);
        samples.push_back(std::move(s));
    }

    std::sort(samples.begin(), samples.end(), [] (const GuitarSample& a, const GuitarSample& b)
    {
        return a.rootMidi < b.rootMidi;
    });
}

const GuitarEngine::GuitarSample* GuitarEngine::nearestSample(int midiNote) const
{
    if (samples.empty())
        return nullptr;

    const GuitarSample* best = &samples.front();
    int bestDistance = std::abs(midiNote - best->rootMidi);

    for (const auto& s : samples)
    {
        const int d = std::abs(midiNote - s.rootMidi);
        if (d < bestDistance)
        {
            best = &s;
            bestDistance = d;
        }
    }
    return best;
}

GuitarEngine::Voice& GuitarEngine::getVoice()
{
    for (auto& v : voices)
        if (!v.active)
            return v;

    auto* quietest = &voices.front();
    for (auto& v : voices)
        if (v.envelope < quietest->envelope)
            quietest = &v;
    return *quietest;
}

GuitarEngine::TransientVoice& GuitarEngine::getTransientVoice()
{
    for (auto& v : transientVoices)
        if (!v.active)
            return v;

    auto* shortest = &transientVoices.front();
    for (auto& v : transientVoices)
        if (v.remainingSamples < shortest->remainingSamples)
            shortest = &v;
    return *shortest;
}

void GuitarEngine::startVoice(int midiNote, float velocity, int delay, float pan, float palmAmount)
{
    if (samples.empty())
        return;

    const int targetMidi = juce::jlimit(24, 105, midiNote + capo);
    const auto* source = nearestSample(targetMidi);
    if (source == nullptr || source->audio.getNumSamples() < 2)
        return;

    auto& v = getVoice();
    v = {};
    v.sample = source;
    v.step = (source->sourceRate / sampleRate)
           * std::pow(2.0, ((double) targetMidi - (double) source->rootMidi) / 12.0);

    std::uniform_real_distribution<float> random(-1.0f, 1.0f);
    v.gain = std::pow(juce::jlimit(0.02f, 1.0f, velocity), 0.72f)
           * (1.0f + random(rng) * 0.035f * humanize);
    v.pan = juce::jlimit(-0.7f, 0.7f, pan + random(rng) * 0.05f * humanize);
    v.delaySamples = juce::jmax(0, delay);
    v.envelope = 1.0f;
    v.releaseMul = 1.0f;
    v.palmAmount = juce::jlimit(0.0f, 1.0f, palmAmount);
    v.palmMul = v.palmAmount < 0.01f
        ? 1.0f
        : coefficientForSeconds(sampleRate,
              juce::jmap(v.palmAmount, 0.0f, 1.0f, 1.30f, 0.055f));
    v.active = true;
}

void GuitarEngine::strumVoicing(const std::array<int, 6>& notes, float velocity, bool down,
                                int baseDelaySamples, float palmOverride)
{
    std::array<int, 6> order { 0, 1, 2, 3, 4, 5 };
    if (!down)
        std::reverse(order.begin(), order.end());

    int activeCount = 0;
    for (int n : notes)
        if (n >= 0) ++activeCount;
    if (activeCount == 0)
        return;

    const float totalSamples = strumMs * 0.001f * (float) sampleRate;
    const float gap = activeCount > 1 ? totalSamples / (float) (activeCount - 1) : 0.0f;
    const float usePalm = palmOverride >= 0.0f ? palmOverride : palmMute;

    std::uniform_real_distribution<float> jitter(-1.0f, 1.0f);
    int played = 0;

    for (int stringIndex : order)
    {
        const int note = notes[(size_t) stringIndex];
        if (note < 0)
            continue;

        const float humanSamples = humanize * 0.0035f * (float) sampleRate * jitter(rng);
        const int delay = juce::jmax(0, baseDelaySamples
                                   + (int) std::round((float) played * gap + humanSamples));
        const float pan = juce::jmap((float) stringIndex, 0.0f, 5.0f, -0.34f, 0.34f);
        const float stringVelocity = juce::jlimit(0.02f, 1.0f,
            velocity * (1.0f + jitter(rng) * 0.025f * humanize));

        startVoice(note, stringVelocity, delay, pan, usePalm);
        ++played;
    }
}

void GuitarEngine::pickString(const std::array<int, 6>& notes, int stringIndex, float velocity,
                              int delaySamples, float palmOverride)
{
    if (stringIndex < 0 || stringIndex >= 6)
        return;

    int idx = stringIndex;
    if (notes[(size_t) idx] < 0)
    {
        for (int distance = 1; distance < 6; ++distance)
        {
            const int hi = idx + distance;
            const int lo = idx - distance;
            if (hi < 6 && notes[(size_t) hi] >= 0) { idx = hi; break; }
            if (lo >= 0 && notes[(size_t) lo] >= 0) { idx = lo; break; }
        }
    }

    if (notes[(size_t) idx] < 0)
        return;

    const float usePalm = palmOverride >= 0.0f ? palmOverride : palmMute;
    const float pan = juce::jmap((float) idx, 0.0f, 5.0f, -0.30f, 0.30f);
    startVoice(notes[(size_t) idx], velocity, delaySamples, pan, usePalm);
}

void GuitarEngine::scratch(bool upward, float intensity, float lengthSeconds, int delaySamples)
{
    auto& t = getTransientVoice();
    t = {};
    t.kind = upward ? TransientKind::scratchUp : TransientKind::scratchDown;
    t.delaySamples = juce::jmax(0, delaySamples);
    t.totalSamples = juce::jmax(64, (int) std::round(sampleRate
                    * juce::jlimit(0.025f, 0.35f, lengthSeconds)));
    t.remainingSamples = t.totalSamples;
    t.gain = juce::jlimit(0.0f, 1.0f, intensity) * 0.50f;
    t.tone = 0.65f;
    t.noiseState ^= (uint32_t) (delaySamples + t.totalSamples * 131);
    t.active = true;
}

void GuitarEngine::percussion(PercussionType type, float velocity, float toneAmount, int delaySamples)
{
    auto& t = getTransientVoice();
    t = {};

    switch (type)
    {
        case PercussionType::body:    t.kind = TransientKind::body; break;
        case PercussionType::thumb:   t.kind = TransientKind::thumb; break;
        case PercussionType::knuckle: t.kind = TransientKind::knuckle; break;
        case PercussionType::slap:    t.kind = TransientKind::slap; break;
    }

    t.delaySamples = juce::jmax(0, delaySamples);
    const float length = type == PercussionType::body ? 0.22f
                       : type == PercussionType::thumb ? 0.16f
                       : type == PercussionType::knuckle ? 0.11f : 0.085f;
    t.totalSamples = juce::jmax(64, (int) std::round(sampleRate * length));
    t.remainingSamples = t.totalSamples;
    t.gain = juce::jlimit(0.0f, 1.0f, velocity) * 0.65f;
    t.tone = juce::jlimit(0.0f, 1.0f, toneAmount);
    t.noiseState ^= (uint32_t) (0x9E3779B9u + delaySamples * 17 + (int) type * 911);
    t.active = true;
}

void GuitarEngine::scheduleChoke(int delaySamples, float releaseSeconds)
{
    for (auto& e : chokeEvents)
    {
        if (!e.active)
        {
            e.delaySamples = juce::jmax(0, delaySamples);
            e.seconds = juce::jlimit(0.004f, 0.25f, releaseSeconds);
            e.active = true;
            return;
        }
    }
    chokeEvents.front() = { juce::jmax(0, delaySamples),
                            juce::jlimit(0.004f, 0.25f, releaseSeconds), true };
}

void GuitarEngine::chokeNow(float releaseSeconds)
{
    releaseAll(releaseSeconds);
}

void GuitarEngine::releaseAll(float seconds)
{
    const float mul = coefficientForSeconds(sampleRate, seconds);
    for (auto& v : voices)
        if (v.active)
            v.releaseMul = mul;
}

void GuitarEngine::process(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (buffer.getNumChannels() < 1 || numSamples <= 0)
        return;

    buffer.clear();

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : left;

    for (int n = 0; n < numSamples; ++n)
    {
        for (auto& e : chokeEvents)
        {
            if (!e.active) continue;
            if (e.delaySamples > 0)
                --e.delaySamples;
            else
            {
                releaseAll(e.seconds);
                e.active = false;
            }
        }

        float l = 0.0f;
        float r = 0.0f;

        for (auto& v : voices)
        {
            if (!v.active || v.sample == nullptr)
                continue;

            if (v.delaySamples > 0)
            {
                --v.delaySamples;
                continue;
            }

            const auto& audio = v.sample->audio;
            const int i0 = (int) v.position;
            if (i0 < 0 || i0 >= audio.getNumSamples() - 1)
            {
                v.active = false;
                continue;
            }

            const int i1 = i0 + 1;
            const float frac = (float) (v.position - (double) i0);
            const float a = audio.getSample(0, i0);
            const float b = audio.getSample(0, i1);
            const float raw = a + (b - a) * frac;
            v.position += v.step;

            const float cutoff = juce::jlimit(850.0f, 17000.0f,
                1600.0f + 15000.0f * tone * (1.0f - 0.78f * v.palmAmount));
            const float pole = std::exp(-2.0f * juce::MathConstants<float>::pi
                                      * cutoff / (float) sampleRate);
            v.lowpass = (1.0f - pole) * raw + pole * v.lowpass;

            v.envelope *= v.releaseMul;
            v.envelope *= v.palmMul;

            if (v.envelope < 0.00012f)
            {
                v.active = false;
                continue;
            }

            const float s = v.lowpass * v.gain * v.envelope;
            const float pan01 = (v.pan + 1.0f) * 0.5f;
            l += s * std::sqrt(1.0f - pan01);
            r += s * std::sqrt(pan01);
        }

        for (auto& t : transientVoices)
        {
            if (!t.active) continue;

            if (t.delaySamples > 0)
            {
                --t.delaySamples;
                continue;
            }

            if (t.remainingSamples <= 0)
            {
                t.active = false;
                continue;
            }

            const float progress = 1.0f - (float) t.remainingSamples
                                             / (float) juce::jmax(1, t.totalSamples);
            float s = 0.0f;

            if (t.kind == TransientKind::scratchDown || t.kind == TransientKind::scratchUp)
            {
                const float noise = nextNoise(t.noiseState);
                const float sweep = t.kind == TransientKind::scratchDown
                                  ? progress : (1.0f - progress);
                const float coeff = juce::jmap(sweep, 0.0f, 1.0f, 0.04f, 0.34f);
                t.filter += coeff * (noise - t.filter);
                const float env = std::sin(juce::MathConstants<float>::pi * progress);
                s = (noise - 0.48f * t.filter) * env * t.gain;
            }
            else
            {
                float hz = 95.0f;
                float noiseAmount = 0.08f;

                switch (t.kind)
                {
                    case TransientKind::body:
                        hz = juce::jmap(t.tone, 0.0f, 1.0f, 72.0f, 125.0f);
                        noiseAmount = 0.035f; break;
                    case TransientKind::thumb:
                        hz = juce::jmap(t.tone, 0.0f, 1.0f, 60.0f, 105.0f);
                        noiseAmount = 0.06f; break;
                    case TransientKind::knuckle:
                        hz = juce::jmap(t.tone, 0.0f, 1.0f, 145.0f, 260.0f);
                        noiseAmount = 0.16f; break;
                    case TransientKind::slap:
                        hz = juce::jmap(t.tone, 0.0f, 1.0f, 190.0f, 360.0f);
                        noiseAmount = 0.36f; break;
                    default: break;
                }

                t.phase += juce::MathConstants<float>::twoPi * hz / (float) sampleRate;
                if (t.phase > juce::MathConstants<float>::twoPi)
                    t.phase -= juce::MathConstants<float>::twoPi;

                const float env = std::exp(-7.5f * progress);
                const float click = nextNoise(t.noiseState) * noiseAmount
                                  * std::exp(-24.0f * progress);
                s = (std::sin(t.phase) * env + click) * t.gain;
            }

            l += s * 0.72f;
            r += s * 0.72f;
            --t.remainingSamples;
        }

        left[n] += l;
        right[n] += r;
    }

    juce::Reverb::Parameters rp;
    rp.roomSize = 0.10f + room * 0.48f;
    rp.damping = 0.50f;
    rp.wetLevel = room * 0.22f;
    rp.dryLevel = 1.0f;
    rp.width = 0.82f;
    rp.freezeMode = 0.0f;
    reverb.setParameters(rp);

    if (buffer.getNumChannels() > 1)
        reverb.processStereo(left, right, numSamples);
    else
        reverb.processMono(left, numSamples);

    buffer.applyGain(juce::Decibels::decibelsToGain(outputDb));
}
