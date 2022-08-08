
struct CubicBezier
{
    CubicBezier(): ax(0.0f), bx(0.0f), cx(0.0f), p1x(0.0f), ay(0.0f), by(0.0f), cy(0.0f), p1y(0)
    {        
    }

    CubicBezier (juce::Point<float> p1, juce::Point<float> p2, juce::Point<float> p3, juce::Point<float> p4)
    {
        /* calculate the polynomial coefficients */
        cx = 3.0f * (p2.getX() - p1.getX());
        bx = 3.0f * (p3.getX() - p2.getX()) - cx;
        ax = p4.getX() - p1.getX() - cx - bx;
        p1x = p1.getX();

        cy = 3.0f * (p2.getY() - p1.getY());
        by = 3.0f * (p3.getY() - p2.getY()) - cy;
        ay = p4.getY() - p1.getY() - cy - by;
        p1y = p1.getY();
    }

    juce::Point<float> getPointOnCubicBezier (float t)
    {
        using namespace chowdsp::Polynomials;

        /* calculate the curve point at parameter value t */
        auto xVal = estrin<3> ({ ax, bx, cx, p1x }, t);
        auto yVal = estrin<3> ({ ay, by, cy, p1y }, t);

        return juce::Point { xVal, yVal };
    }

    float ax, bx, cx, p1x;
    float ay, by, cy, p1y;
};
