/*
 *  Copyright (c) 2015 Jouni Pentikäinen <joupent@gmail.com>
 *  Copyright (c) 2020 Emmet O'Neill <emmetoneill.pdx@gmail.com>
 *  Copyright (c) 2020 Eoin O'Neill <eoinoneill1991@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KIS_KEYFRAME_CHANNEL_H
#define KIS_KEYFRAME_CHANNEL_H

#include <QVariant>
#include <QDomElement>
#include <kundo2command.h>

#include "kis_types.h"
#include "KoID.h"
#include "kis_keyframe.h"
#include "kis_default_bounds.h"
#include "kis_default_bounds_node_wrapper.h"

#include "kritaimage_export.h"

class KisTimeSpan;


/** @brief KisKeyframeChannel stores and manages KisKeyframes.
 * Maps units of time to virtual keyframe values.
 * This class is a key piece of Krita's animation backend.
 * Abstract base class of KisRasterKeyframeChannel, KisScalarKeyframeChannel, etc.
 */
class KRITAIMAGE_EXPORT KisKeyframeChannel : public QObject
{
    Q_OBJECT

public:
    static const KoID Raster;
    static const KoID Opacity;
    static const KoID TransformArguments;
    static const KoID TransformPositionX;
    static const KoID TransformPositionY;
    static const KoID TransformScaleX;
    static const KoID TransformScaleY;
    static const KoID TransformShearX;
    static const KoID TransformShearY;
    static const KoID TransformRotationX;
    static const KoID TransformRotationY;
    static const KoID TransformRotationZ;

    Q_DECL_DEPRECATED KisKeyframeChannel(const KoID &id, KisNodeWSP parent = 0);
    KisKeyframeChannel(const KoID &id, KisDefaultBoundsBaseSP bounds);
    KisKeyframeChannel(const KisKeyframeChannel &rhs, KisNodeWSP newParent);
    ~KisKeyframeChannel() override;

    /** @brief Add a new keyframe to the channel at the specified time. */
    void addKeyframe(int time, KUndo2Command *parentUndoCmd = nullptr);

    /** @brief Insert an existing keyframe into the channel at the specified time. */
    void insertKeyframe(int time, KisKeyframeSP keyframe, KUndo2Command *parentUndoCmd = nullptr);

    /** @brief Remove a keyframe from the channel at the specified time. */
    void removeKeyframe(int time, KUndo2Command *parentUndoCmd = nullptr);

    // Inter-channel operations..
    /** @brief Move a keyframe across channel(s) at the specified times. */
    static void moveKeyframe(KisKeyframeChannel *sourceChannel, int sourceTime, KisKeyframeChannel *targetChannel, int targetTime, KUndo2Command* parentUndoCmd = nullptr);

    /** @brief Copy a keyframe across channel(s) at the specified times. */
    static void copyKeyframe(KisKeyframeChannel *sourceChannel, int sourceTime, KisKeyframeChannel *targetChannel, int targetTime, KUndo2Command* parentUndoCmd = nullptr);

    /** @brief Swap two keyframes across channel(s) at the specified times. */
    static void swapKeyframes(KisKeyframeChannel *channelA, int timeA, KisKeyframeChannel *channelB, int timeB, KUndo2Command* parentUndoCmd = nullptr);

    // Intra-channel convenience methods..
    void moveKeyframe(int sourceTime, int targetTime, KUndo2Command* parentUndoCmd = nullptr) { moveKeyframe(this, sourceTime, this, targetTime, parentUndoCmd); }
    void copyKeyframe(int sourceTime, int targetTime, KUndo2Command* parentUndoCmd = nullptr) { copyKeyframe(this, sourceTime, this, targetTime, parentUndoCmd); }
    void swapKeyframes(int timeA, int timeB, KUndo2Command* parentUndoCmd = nullptr) { swapKeyframes(this, timeA, this, timeB, parentUndoCmd); }

    // Keyframe methods..
    /** @brief Get a keyframe at specified time.
     * Used primarily when the value of a given keyframe is needed. */
    KisKeyframeSP keyframeAt(int time) const;
    KisKeyframeSP activeKeyframeAt(int time) const { return keyframeAt(activeKeyframeTime(time)); }

    // Quick casting convenience templates..
    template <class KeyframeType>
    QSharedPointer<KeyframeType> keyframeAt(int time) const {
        return keyframeAt(time).dynamicCast<KeyframeType>();
    }

    template <class KeyframeType>
    QSharedPointer<KeyframeType> activeKeyframeAt(int time) const {
        return activeKeyframeAt(time).dynamicCast<KeyframeType>();
    }

    int keyframeCount() const;

    // Time methods..
    /** @brief Get the time of the active keyframe.
     * Useful for snapping any time to that of the most recent keyframe.
     * @param  time  Input time. When not specified, currentTime() will be used. */
    int activeKeyframeTime(int time) const;
    int activeKeyframeTime() const { return activeKeyframeTime(currentTime()); }

    int firstKeyframeTime() const;
    int previousKeyframeTime(const int time) const;
    int nextKeyframeTime(const int time) const;
    int lastKeyframeTime() const;

    /** @brief Get a set of all integer times that map to a keyframe. */
    QSet<int> allKeyframeTimes() const;

    QString id() const;
    QString name() const;

    Q_DECL_DEPRECATED void setNode(KisNodeWSP node);
    Q_DECL_DEPRECATED KisNodeWSP node() const;

    /** @brief Calculates a pseudo-unique hash based on
     * the relevant internal state of the channel. */
    int channelHash() const;

    /** @brief Get the set of frames affected by any changes to the value
     * of the active keyframe at the given time. */
    KisTimeSpan affectedFrames(int time) const; //TEMP NOTE: scalar specific?

    /** @brief Get a span of times for which the channel gives identical
     * results compared to frame at a given time.
     * NOTE: This set may be different than the set of affected frames
     * due to interpolation. */
    KisTimeSpan identicalFrames(int time) const; //TEMP NOTE: scalar specific?

    virtual QDomElement toXML(QDomDocument doc, const QString &layerFilename);
    virtual void loadXML(const QDomElement &channelNode);

Q_SIGNALS:
    /** @brief This signal is emitted whenever the relevant internal state
     * of the channel is changed.
     * @param  affectedTimeSpan  The span of times that were affected by the change.
     * @param  affectedArea  The area of the paint device that were affected by the change. */
    void sigChannelUpdated(const KisTimeSpan &affectedTimeSpan, const QRect &affectedArea) const;

    /** @brief This signal is emitted just AFTER a keyframe was added to the channel. */
    void sigAddedKeyframe(const KisKeyframeChannel *channel, int time);

    /** @brief This signal is emitted just BEFORE a keyframe is removed from the channel. */
    void sigRemovingKeyframe(const KisKeyframeChannel *channel, int time);

protected:
    typedef QMap<int, KisKeyframeSP> TimeKeyframeMap;
    TimeKeyframeMap &keys();
    const TimeKeyframeMap &constKeys() const;

    int currentTime() const;

    Q_DECL_DEPRECATED void workaroundBrokenFrameTimeBug(int *time); //TEMP NOTE: scalar specific?

private:
    struct Private;
    QScopedPointer<Private> m_d;

    TimeKeyframeMap::const_iterator activeKeyIterator(int time) const;

    /** @brief Virtual keyframe creation function.
     * Derived classes implement this function based on the needs
     * of their specific KisKeyframe subclasses. */
    virtual KisKeyframeSP createKeyframe() = 0;
    virtual QRect affectedRect(int time) const = 0;
    virtual QPair<int, KisKeyframeSP> loadKeyframe(const QDomElement &keyframeNode) = 0;
    virtual void saveKeyframe(KisKeyframeSP keyframe, QDomElement keyframeElement, const QString &layerFilename) = 0;
};

#endif // KIS_KEYFRAME_CHANNEL_H
