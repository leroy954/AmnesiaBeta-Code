"""Tiny client for the unreal-mcp HTTP server, so scripts can drive the editor directly."""
import json, urllib.request, itertools, time

URL = "http://127.0.0.1:8000/mcp"
_ids = itertools.count(1)
_session = None

S = "editor_toolset.toolsets.scene.SceneTools"
A = "editor_toolset.toolsets.actor.ActorTools"
O = "editor_toolset.toolsets.object.ObjectTools"
AS = "editor_toolset.toolsets.asset.AssetTools"
E = "EditorToolset.EditorAppToolset"


def _post(payload, timeout=600):
    global _session
    headers = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}
    if _session:
        headers["Mcp-Session-Id"] = _session
    req = urllib.request.Request(URL, data=json.dumps(payload).encode(), headers=headers, method="POST")
    with urllib.request.urlopen(req, timeout=timeout) as r:
        sid = r.headers.get("Mcp-Session-Id")
        if sid:
            _session = sid
        body = r.read().decode()
    return json.loads(body) if body.strip() else None


def connect():
    _post({"jsonrpc": "2.0", "id": next(_ids), "method": "initialize",
           "params": {"protocolVersion": "2025-03-26", "capabilities": {}, "clientInfo": {"name": "cc", "version": "1"}}})
    _post({"jsonrpc": "2.0", "method": "notifications/initialized"})


def call(toolset, tool, **args):
    """Calls a toolset tool and returns its decoded returnValue."""
    if _session is None:
        connect()
    res = _post({"jsonrpc": "2.0", "id": next(_ids), "method": "tools/call",
                 "params": {"name": "call_tool", "arguments": {"toolset_name": toolset, "tool_name": tool, "arguments": args}}})
    if "error" in res:
        raise RuntimeError(str(res["error"])[:600])
    result = res["result"]
    text = "".join(c.get("text", "") for c in result.get("content", []))
    if result.get("isError"):
        raise RuntimeError(text[:800])
    try:
        data = json.loads(text)
    except Exception:
        return text
    if isinstance(data, dict) and "returnValue" in data:
        return data["returnValue"]
    return data


def xform(x=0, y=0, z=0, pitch=0, yaw=0, roll=0, sx=1, sy=1, sz=1):
    return {"location": {"x": x, "y": y, "z": z}, "rotation": {"pitch": pitch, "yaw": yaw, "roll": roll},
            "scale": {"x": sx, "y": sy, "z": sz}}


def spawn(asset_path, name, x=0, y=0, z=0, yaw=0, pitch=0, roll=0, scale=1.0, sx=None, sy=None, sz=None, folder=None, snap=False):
    a = call(S, "add_to_scene_from_asset", asset_path=asset_path, name=name,
             xform=xform(x, y, z, pitch, yaw, roll, sx if sx else scale, sy if sy else scale, sz if sz else scale),
             snap_to_ground=snap)
    if a and folder:
        call(S, "set_actor_folder", actor=a, folder_path=folder)
    return a


def props(obj, **values):
    return call(O, "set_properties", instance=obj, values=json.dumps(values))


def get_props(obj, *names):
    r = call(O, "get_properties", instance=obj, properties=list(names))
    return json.loads(r) if isinstance(r, str) else r


_BATCH_SCRIPT = '''
import json
def run():
    S='editor_toolset.toolsets.scene.SceneTools.'
    A='editor_toolset.toolsets.actor.ActorTools.'
    O='editor_toolset.toolsets.object.ObjectTools.'
    items = json.loads(DATA)
    made = 0
    errs = []
    for it in items:
        try:
            a = execute_tool(S+'add_to_scene_from_asset', json.dumps({'asset_path': it['asset'], 'name': it.get('name','Item'),
                'xform': {'location': {'x': it['x'], 'y': it['y'], 'z': it['z']},
                          'rotation': {'pitch': it.get('pitch',0), 'yaw': it.get('yaw',0), 'roll': it.get('roll',0)},
                          'scale': {'x': it['sx'], 'y': it['sy'], 'z': it['sz']}}}))['returnValue']
            if a is None:
                errs.append('none:' + it['asset'][-30:]); continue
            r = json.dumps({'actor': a})
            if it.get('label'):
                execute_tool(A+'set_label', json.dumps({'actor': a, 'label': it['label']}))
            if it.get('folder'):
                execute_tool(S+'set_actor_folder', json.dumps({'actor': a, 'folder_path': it['folder']}))
            for t in it.get('tags', []):
                execute_tool(A+'add_tag', json.dumps({'actor': a, 'tag': t}))
            if it.get('props') or it.get('comp'):
                comps = execute_tool(A+'get_components', r)['returnValue']
                c = [x for x in comps if 'StaticMeshComponent' in x['refPath'] or 'SkeletalMeshComponent' in x['refPath']]
                if c:
                    if it.get('props'):
                        execute_tool(O+'set_properties', json.dumps({'instance': c[0], 'values': json.dumps(it['props'])}))
            made += 1
        except Exception as e:
            errs.append(str(e)[:120])
    return {'made': made, 'errs': errs[:6], 'nerr': len(errs)}
'''


def batch_spawn(items, chunk=120):
    """Spawns many static mesh actors inside one editor-side script (fast, no per-call frame wait)."""
    made = 0
    for i in range(0, len(items), chunk):
        part = items[i:i + chunk]
        code = "DATA = " + repr(json.dumps(part)) + "\n" + _BATCH_SCRIPT
        r = call("editor_toolset.toolsets.programmatic.ProgrammaticToolset", "execute_tool_script", script=code)
        res = json.loads(r) if isinstance(r, str) else r
        made += res["made"]
        if res["nerr"]:
            print("  batch errors:", res["nerr"], res["errs"])
    return made


def item(asset, x, y, z, yaw=0, scale=1.0, sx=None, sy=None, sz=None, pitch=0, roll=0, name="Item", folder=None, tags=None, props=None, label=None):
    return {"asset": asset, "x": x, "y": y, "z": z, "yaw": yaw, "pitch": pitch, "roll": roll,
            "sx": sx if sx else scale, "sy": sy if sy else scale, "sz": sz if sz else scale,
            "name": name, "folder": folder, "tags": tags or [], "props": props, "label": label or name}


def asset_path(p):
    """'/Game/X/Y' -> '/Game/X/Y.Y'"""
    return p + "." + p.split("/")[-1]


JOBDIR = r"C:\Users\User\Documents\Unreal Projects\AmnesiaBeta\Saved\LevelJobs"


def job(ops, timeout=600, chunk=1500, world=None):
    """Runs ops through the editor-side level job watcher (FAmnesiaLevelJobs). Returns merged results."""
    import os, time as _t
    total = {"spawned": 0, "deleted": 0, "modified": 0, "errors": [], "imported": [], "report": []}
    os.makedirs(JOBDIR, exist_ok=True)
    for i in range(0, len(ops), chunk):
        part = ops[i:i + chunk]
        res = os.path.join(JOBDIR, "result.json")
        if os.path.exists(res):
            os.remove(res)
        tmp = os.path.join(JOBDIR, "job.tmp")
        with open(tmp, "w", encoding="utf-8") as f:
            json.dump({"ops": part, "world": world} if world else {"ops": part}, f)
        os.replace(tmp, os.path.join(JOBDIR, "job.json"))
        t0 = _t.time()
        while not os.path.exists(res):
            _t.sleep(0.5)
            if _t.time() - t0 > timeout:
                raise RuntimeError("job timed out")
        _t.sleep(0.3)
        with open(res, encoding="utf-8") as f:
            r = json.load(f)
        if "error" in r:
            raise RuntimeError(r["error"])
        for k in ("spawned", "deleted", "modified"):
            total[k] += r[k]
        total["errors"] += r["errors"]
        total["imported"] += r.get("imported", [])
        total["report"] += r.get("report", [])
    total["errors"] = sorted(set(total["errors"]))[:8]
    return total


def sm(mesh, x, y, z, yaw=0, s=1.0, sx=None, sy=None, sz=None, pitch=0, roll=0, label=None, folder=None, tags=None, collision=None, hidden=False, material=None, shadow=None):
    """Static mesh spawn op for job()."""
    o = {"op": "spawn", "mesh": mesh, "loc": [x, y, z], "rot": [pitch, yaw, roll],
         "scale": [sx or s, sy or s, sz or s]}
    if label: o["label"] = label
    if folder: o["folder"] = folder
    if tags: o["tags"] = tags
    if collision: o["collision"] = collision
    if hidden: o["hidden"] = True
    if material: o["material"] = material
    if shadow is not None: o["shadow"] = shadow
    return o


def shot(path, tool="CaptureEditorImage", **args):
    """Captures the editor window (or viewport) to a PNG file."""
    import base64, ctypes, subprocess
    try:
        r = call(E, tool, **args)
    except RuntimeError:
        # the editor window got minimized: restore it without stealing focus, then retry
        ps = ('Add-Type -MemberDefinition \'[DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h,int c);\' -Name W -Namespace U; '
              'Get-Process UnrealEditor | ForEach-Object { [U.W]::ShowWindow($_.MainWindowHandle,4) | Out-Null }')
        subprocess.run(["powershell", "-NoProfile", "-Command", ps], capture_output=True)
        import time as _t
        _t.sleep(3)
        r = call(E, tool, **args)
    img = r["image"] if isinstance(r, dict) and "image" in r else r
    open(path, "wb").write(base64.b64decode(img["data"]))
    return path


def sheet(assets, out, cols=4, size=300):
    """Renders asset thumbnails into one labelled contact sheet PNG."""
    import base64, io
    from PIL import Image, ImageDraw
    rows = (len(assets) + cols - 1) // cols
    img = Image.new("RGB", (cols * size, rows * size), (30, 30, 34))
    d = ImageDraw.Draw(img)
    for i, a in enumerate(assets):
        try:
            r = call(E, "CaptureAssetImage", assetPath=a)
            t = Image.open(io.BytesIO(base64.b64decode(r["data"]))).convert("RGB").resize((size, size))
            img.paste(t, ((i % cols) * size, (i // cols) * size))
        except Exception as e:
            d.text(((i % cols) * size + 5, (i // cols) * size + 20), "ERR " + str(e)[:40], fill=(255, 80, 80))
        d.text(((i % cols) * size + 6, (i // cols) * size + 4), a.split("/")[-1].split(".")[0], fill=(255, 255, 255))
    img.save(out)
    return out


def pie_on():
    return call(E, "IsPIERunning")


def mesh_comp(actor):
    for c in call(A, "get_components", actor=actor):
        if "StaticMeshComponent" in c["refPath"]:
            return c
    return None
