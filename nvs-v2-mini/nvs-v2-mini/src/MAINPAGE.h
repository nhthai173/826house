#ifndef MAINPAGE_H
#define MAINPAGE_H

const char *MAINPAGE = R"html(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH" crossorigin="anonymous"><title>Nha ve sinh</title></head><body><div class="container-fluid" style="max-width:600px"><div class="row justify-content-center bg-light p-3 rounded mt-xl-5 mt-3"><div class="col"><div class="d-flex"><a class="btn btn-secondary rounded-pill" href="/update">Cập nhật firmware</a><a class="ms-2 btn btn-info rounded-pill" href="/restart">Restart</a></div></div></div><div class="row justify-content-center mt-4"><div class="col"><table class="table" id="table"><thead><tr><th>Tên</th><th>Trạng thái</th><th>Hành động</th></tr></thead><tbody></tbody></table></div></div></div><script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js" integrity="sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz" crossorigin="anonymous"></script><script>var DATA = {}
var ws = null
const addRow = (data) => {
    const $tb = document.querySelector('#table tbody')
    const $tr = document.createElement('tr')
    $tr.setAttribute('data-name', data.name)
    $tr.innerHTML = `<td>${data.name}</td><td>${data.status}</td><td></td>`
    $tb.appendChild($tr)
}
const updateRow = (data) => {
    const $tr = document.querySelector(`#table tbody tr[data-name="${data.name}"]`)
    if (!$tr) return addRow(data)
    $tr.querySelector('td:nth-child(2)').innerHTML = data.status
}
const updateAction = (name, action) => {
    const $tr = document.querySelector(`#table tbody tr[data-name="${name}"]`)
    if (!$tr) return
    const $td = $tr.querySelector('td:nth-child(3)')
    if (action == 'ACTION_ONOFF') {
        const btn = document.createElement('div')
        btn.className = "form-check form-switch"
        btn.innerHTML = `<input class="form-check-input" type="checkbox" role="switch">`
        $td.appendChild(btn)
        btn.querySelector('input').addEventListener('change', (e) => {
            console.log(e.target.checked)
            wsSend(`${name}:${e.target.checked ? 'ON' : 'OFF'}`)
        })
    } else if (action.startsWith('ACTION_SET')) {
        const btn = document.createElement('button')
        btn.className = "btn btn-sm btn-primary"
        btn.innerHTML = `Sửa`
        $td.appendChild(btn)
        btn.addEventListener('click', () => {
            const value = prompt('Nhập giá trị mới')
            if (action.includes('NUMBER')) {
                if (isNaN(value)) return alert('Giá trị không hợp lệ. Vui lòng nhập số')
            }
            if (value) wsSend(`${name}:${value}`)
        })
    }
}
const wsSend = (data) => {
    if (!ws) return
    ws.send(data)
}
const connectWS = () => {
    try { ws = new WebSocket(`ws://${location.host}/ws`) } catch (e) {}
    if (!ws) {
        return setTimeout(3000, connectWS)
    }
    ws.onopen = () => {
        console.log('Connected to server')
        ws.send(`Hello from ` + navigator.userAgent)
    }
    ws.onclose = () => {
        console.log('Disconnected from server')
        setTimeout(5000, connectWS)
    }
    ws.onmessage = (msg) => {
        console.log(msg.data)
        if (!msg.data) return
        try {
            msg.data.split('\r\n').forEach(d => {
                if (!d) return
                const [key, value] = d.split(':')
                if (!value) return
                if (value.startsWith('ACTION_')) {
                    updateAction(key, value)
                } else {
                    updateRow({ name: key, status: value })
                }
            })
        } catch (e) {
            console.error('[ws][onmessage]', e)
        }
    }
}
connectWS()</script></body></html>
)html";

#endif // MAINPAGE_H