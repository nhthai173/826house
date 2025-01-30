## Connections

Defined in `pinconfig.h`

<table>
    <tbody>
        <tr>
            <th>Pin</th>
            <th>Function</th>
            <th>Note</th>
        </tr>
        <tr>
            <th colspan="3">Output</th>
        </tr>
        <tr>
            <td>4</td>
            <td>Relay 1</td>
            <td>active low, 16A</td>
        </tr>
        <tr>
            <td>5</td>
            <td>Relay 2</td>
            <td>active high, 10A</td>
        </tr>
        <tr>
            <td>16</td>
            <td>Relay 3</td>
            <td>active low, 10A</td>
        </tr>
        <tr>
            <th colspan="3">Input</th>
        </tr>
        <tr>
            <td>14</td>
            <td>R1 button</td>
            <td>active low</td>
        </tr>
        <tr>
            <td>12</td>
            <td>R2 button</td>
            <td>active low</td>
        </tr>
        <tr>
            <td>13</td>
            <td>R3 button</td>
            <td>active low</td>
        </tr>
    </tbody>
</table>

## Changelog

### 1.0.3

    30 Jan 2025
    Firmware version: 3

- Improved button response times for R1, R2, and R3 buttons (by using interrupt).
- Fixed minor bugs related to input signal processing.
- Enhanced overall system stability and performance.
- Supported auto power off
- Supported local control API