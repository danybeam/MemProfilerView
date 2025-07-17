export module Maths:Tweening;

import :Easing;

export namespace mem_profile_viewer
{
    class Tween final
    {
    public:
        Tween(float& reference);
        ~Tween() = default;

        void on_update(float delta);

        bool is_running() const;

        void start(float target, float duration, const EasingFunction& easing_function = Linear::Interpolation);
        void start_with_delay(
            float target,
            float duration,
            float delay,
            const EasingFunction& easing_function = Linear::Interpolation
        );

        void cancel();

        [[nodiscard]] float m_target() const
        {
            return m_target_;
        }

        [[nodiscard]] bool m_is_running() const
        {
            return m_is_running_;
        }

        [[nodiscard]] float remaining() const
        {
            return m_is_running_ ? m_duration_ - m_elapsed_ : 0;
        }

    protected:
        float& m_reference_;
        float m_elapsed_;
        float m_duration_;
        float m_start_;
        float m_target_;
        float m_delay_;
        bool m_is_running_;
        EasingFunction m_easing_function_;
    };
}

mem_profile_viewer::Tween::Tween(float& reference) :
    m_reference_(reference),
    m_elapsed_(0.0f),
    m_duration_(0.0f),
    m_start_(0.0f),
    m_target_(0.0f),
    m_delay_(0.0f),
    m_is_running_(false),
    m_easing_function_(Linear::Interpolation)
{
}

void mem_profile_viewer::Tween::on_update(float delta)
{
    if (m_delay_ > 0.0f)
    {
        m_delay_ -= delta;
        if (m_delay_ < 0.0f)
        {
            m_delay_ = 0.0f;
        }
        else
        {
            return;
        }
    }

    if (m_is_running_)
    {
        if (m_elapsed_ < m_duration_)
        {
            m_elapsed_ += delta;
            if (m_elapsed_ > m_duration_)
            {
                m_elapsed_ = m_duration_;
                m_is_running_ = false;
            }

            float pct = m_elapsed_ / m_duration_;
            m_reference_ = m_start_ + (m_target_ - m_start_) * m_easing_function_(pct);
        }
    }
}

bool mem_profile_viewer::Tween::is_running() const
{
    return m_is_running_;
}

void mem_profile_viewer::Tween::start(const float target, const float duration, const EasingFunction& easing_function)
{
    cancel();

    m_duration_ = duration;
    m_start_ = m_reference_;
    m_target_ = target;
    m_is_running_ = true;
    m_easing_function_ = easing_function;
}

void mem_profile_viewer::Tween::start_with_delay(const float target, const float duration, const float delay,
                                                 const EasingFunction& easing_function)
{
    cancel();

    m_delay_ = delay;
    m_duration_ = duration;
    m_start_ = m_reference_;
    m_target_ = target;
    m_is_running_ = true;
    m_easing_function_ = easing_function;
}

void mem_profile_viewer::Tween::cancel()
{
    m_elapsed_ = 0.0f;
    m_duration_ = 0.0f;
    m_start_ = 0.0f;
    m_target_ = 0.0f;
    m_delay_ = 0.0f;
    m_is_running_ = false;
    m_easing_function_ = Linear::Interpolation;
}
