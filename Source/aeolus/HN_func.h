//
// Created by Wally Young on 6/27/25.
//

#include "globals.h"
#include "N_func.h"

AEOLUS_NAMESPACE_BEGIN

/**
 * @brief Interpolated per-note look-up table for harmonics.
 *
 * This class keeps a per-note LUT for each of the N_HARM harmonics.
 */
    class HN_func final
    {
    public:
        HN_func();
        void reset(float v);
        void setValue(int idx, float v);            // setv(i, v)
        void setValue(int harm, int idx, float v);  // setv(h, i, v)
        void clearValue(int idx);                   // clrv(i)
        void clearValue(int harm, int idx);         // clrv(h, i);
        float getValue(int harm, int idx) const;    // vs(h, i);
        bool isSet(int harm, int idx) const;        // st(h, i)

        N_func& operator[](int harm) { assert(isPositiveAndBelow(harm, _h.size())); return _h[harm]; }
        const N_func& operator[](int harm) const { assert(isPositiveAndBelow(harm, _h.size())); return _h[harm]; }

        void fromJson(const nlohmann::json& v);

        void read(std::istream& stream, int n = N_HARM);

    private:
        std::array<N_func, N_HARM> _h;
    };

AEOLUS_NAMESPACE_END