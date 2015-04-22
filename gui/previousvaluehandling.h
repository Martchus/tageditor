#ifndef QTGUI_PREVIOUSVALUEHANDLING_H
#define QTGUI_PREVIOUSVALUEHANDLING_H

namespace QtGui {

/*!
 * \brief Specifies the "previous value handling" policy.
 */
enum class PreviousValueHandling : int
{
    Auto, /**< The policy is determined automatically. */
    Clear, /**< The previous value will be cleared. */
    Update, /**< The previous value will be updated. */
    IncrementUpdate, /**< The previous value will be updated or - if possible - incremented. */
    Keep /**< The previous value will be kept. */
};

}

#endif // QTGUI_PREVIOUSVALUEHANDLING_H
