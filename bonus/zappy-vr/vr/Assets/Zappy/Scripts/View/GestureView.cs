using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Zappy.Model;

namespace Zappy.View
{
    /// <summary>
    /// Short procedural gestures acting out what a trantorian does:
    ///  - collecting food/minerals: it bows, the morsel rises from the ground to its
    ///    mouth and is eaten in shrinking bites with little chewing nods;
    ///  - broadcasting: a mouth opens and pulses while a hand rises to it and the
    ///    robot leans back to shout.
    /// Props are parented to the robot's Body so they follow its heading and bob.
    /// </summary>
    public class GestureView : MonoBehaviour
    {
        private static readonly Color MouthColor = new Color(0.08f, 0.06f, 0.06f);

        private GameState _state;
        private EntitiesView _entities;
        private readonly Dictionary<int, long> _seenStamps = new();
        private long _seenBroadcast = -1;
        private bool _primed;

        public void Bind(GameState state, EntitiesView entities)
        {
            _state = state;
            _entities = entities;
            _seenStamps.Clear();
            _seenBroadcast = -1;
            _primed = false;
        }

        private void Update()
        {
            if (_state == null)
                return;
            if (!_primed)
            {
                // Swallow everything that happened before we joined.
                foreach (KeyValuePair<int, PlayerData> pair in _state.Players)
                    _seenStamps[pair.Key] = pair.Value.ActionStamp;
                foreach (BroadcastEntry broadcast in _state.Broadcasts)
                    _seenBroadcast = broadcast.Sequence;
                _primed = true;
                return;
            }

            foreach (KeyValuePair<int, PlayerData> pair in _state.Players)
            {
                if (pair.Value.ActionStamp == _seenStamps.GetValueOrDefault(pair.Key))
                    continue;
                _seenStamps[pair.Key] = pair.Value.ActionStamp;
                if (pair.Value.LastAction == "Collected resource")
                    StartCoroutine(EatGesture(pair.Key, pair.Value.LastResource));
            }

            foreach (BroadcastEntry broadcast in _state.Broadcasts)
            {
                if (broadcast.Sequence <= _seenBroadcast)
                    continue;
                _seenBroadcast = broadcast.Sequence;
                StartCoroutine(ShoutGesture(broadcast.PlayerId));
            }
        }

        private IEnumerator EatGesture(int playerId, int resourceIndex)
        {
            if (!_entities.TryGetGestureRig(playerId, out Transform body, out Transform model, out int level))
                yield break;
            float height = EntitiesView.RobotHeight(level);
            bool food = resourceIndex == ResourceInfo.FoodIndex;
            Color color = ResourceInfo.Colors[Mathf.Clamp(resourceIndex, 0, ResourceInfo.Count - 1)];

            GameObject morsel = GameObject.CreatePrimitive(food ? PrimitiveType.Cylinder : PrimitiveType.Cube);
            morsel.name = "Morsel";
            Destroy(morsel.GetComponent<Collider>());
            morsel.transform.SetParent(body, false);
            float size = height * 0.16f;
            Vector3 fullScale = new Vector3(size, food ? size * 0.5f : size, size);
            morsel.transform.localScale = fullScale;
            morsel.GetComponent<Renderer>().sharedMaterial = MaterialCache.GetLit(color);

            var ground = new Vector3(0f, height * 0.05f, height * 0.45f);
            var mouth = new Vector3(0f, height * 0.78f, height * 0.30f);

            // Bow into the grab and carry the morsel up to the mouth.
            for (float t = 0f; t < 1f; t += Time.deltaTime / 0.45f)
            {
                if (morsel == null || model == null)
                    yield break;
                float s = Mathf.SmoothStep(0f, 1f, t);
                morsel.transform.localPosition =
                    Vector3.Lerp(ground, mouth, s) + Vector3.forward * (Mathf.Sin(s * Mathf.PI) * height * 0.12f);
                model.localRotation = Quaternion.Euler(Mathf.Sin(s * Mathf.PI) * 18f, 0f, 0f);
                yield return null;
            }

            // Three shrinking bites with a chewing nod each.
            for (int bite = 2; bite >= 0; bite--)
            {
                if (morsel == null)
                    break;
                morsel.transform.localPosition = mouth;
                morsel.transform.localScale = fullScale * (bite / 3f + 0.05f);
                if (model != null)
                    model.localRotation = Quaternion.Euler(7f, 0f, 0f);
                yield return new WaitForSeconds(0.09f);
                if (model != null)
                    model.localRotation = Quaternion.identity;
                yield return new WaitForSeconds(0.09f);
            }

            if (morsel != null)
                Destroy(morsel);
            if (model != null)
                model.localRotation = Quaternion.identity;
        }

        private IEnumerator ShoutGesture(int playerId)
        {
            if (!_entities.TryGetGestureRig(playerId, out Transform body, out Transform model, out int level))
                yield break;
            float height = EntitiesView.RobotHeight(level);

            GameObject mouth = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            mouth.name = "Mouth";
            Destroy(mouth.GetComponent<Collider>());
            mouth.transform.SetParent(body, false);
            mouth.transform.localPosition = new Vector3(0f, height * 0.78f, height * 0.32f);
            mouth.transform.localScale = Vector3.zero;
            mouth.GetComponent<Renderer>().sharedMaterial = MaterialCache.GetLit(MouthColor);

            GameObject hand = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            hand.name = "Hand";
            Destroy(hand.GetComponent<Collider>());
            hand.transform.SetParent(body, false);
            hand.transform.localScale = Vector3.one * (height * 0.14f);
            hand.GetComponent<Renderer>().sharedMaterial =
                MaterialCache.GetLit(Color.Lerp(LevelStyle.ColorOf(level), Color.black, 0.25f));

            var rest = new Vector3(height * 0.38f, height * 0.35f, height * 0.10f);
            var raised = new Vector3(height * 0.20f, height * 0.72f, height * 0.30f);
            Vector3 mouthOpen = new Vector3(height * 0.16f, height * 0.13f, height * 0.10f);

            // Raise the hand, open the mouth, lean back.
            for (float t = 0f; t < 1f; t += Time.deltaTime / 0.25f)
            {
                if (mouth == null || hand == null || model == null)
                    yield break;
                float s = Mathf.SmoothStep(0f, 1f, t);
                hand.transform.localPosition = Vector3.Lerp(rest, raised, s);
                mouth.transform.localScale = Vector3.Scale(mouthOpen, new Vector3(1f, s, 1f));
                model.localRotation = Quaternion.Euler(-10f * s, 0f, 0f);
                yield return null;
            }

            // Shout: the mouth pulses while the bubble/ring effects play.
            for (float t = 0f; t < 0.9f; t += Time.deltaTime)
            {
                if (mouth == null)
                    yield break;
                float pulse = 0.65f + 0.35f * Mathf.Abs(Mathf.Sin(t * 18f));
                mouth.transform.localScale = Vector3.Scale(mouthOpen, new Vector3(1f, pulse, 1f));
                yield return null;
            }

            // Ease back to rest.
            for (float t = 0f; t < 1f; t += Time.deltaTime / 0.25f)
            {
                if (mouth == null || hand == null || model == null)
                    break;
                float s = Mathf.SmoothStep(1f, 0f, t);
                hand.transform.localPosition = Vector3.Lerp(rest, raised, s);
                mouth.transform.localScale = Vector3.Scale(mouthOpen, new Vector3(1f, s, 1f));
                model.localRotation = Quaternion.Euler(-10f * s, 0f, 0f);
                yield return null;
            }

            if (mouth != null)
                Destroy(mouth);
            if (hand != null)
                Destroy(hand);
            if (model != null)
                model.localRotation = Quaternion.identity;
        }
    }
}
