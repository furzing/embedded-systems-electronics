Option Explicit

Function LoRa_SF12_BW125_CommConstraint(callMode, stateData)
    Dim m
    m = LCase(CStr(callMode))
    If m = "register" Then
        LoRa_SF12_BW125_CommConstraint = Register()
        Exit Function
    End If
    If m = "compute" Then
        LoRa_SF12_BW125_CommConstraint = Compute(stateData)
        Exit Function
    End If
    LoRa_SF12_BW125_CommConstraint = Empty
End Function

Function Register()
    Dim inputs(2)
    inputs(0) = "ArgumentType = Input"
    inputs(1) = "Type = Value"
    inputs(2) = "Name = CarrierPower"
    
    Dim outputs(2)
    outputs(0) = "ArgumentType = Output"
    outputs(1) = "Type = Value"
    outputs(2) = "Name = PluginConstraintValue"
    
    Register = Array(inputs, outputs)
End Function

Function Compute(stateData)
    Dim carrierP_dBW, margin_dB
    carrierP_dBW = CDbl(stateData(0))
    margin_dB = carrierP_dBW - (-171)
    Compute = Array(margin_dB)
End Function