#include "can_stm32_bxcan.h"
#include "stm32f1xx_hal_can.h"

int bx_init(struct bxcan_driver *drv, uint32_t bitrate)
{
    if (!drv || !drv->hcan)
        return -1;

    CAN_HandleTypeDef *h = drv->hcan;

    /* Common nominal settings */
    h->Init.Mode = CAN_MODE_NORMAL;
    h->Init.SyncJumpWidth = CAN_SJW_1TQ;
    h->Init.TimeSeg1 = CAN_BS1_8TQ;
    h->Init.TimeSeg2 = CAN_BS2_3TQ;
    h->Init.AutoBusOff = DISABLE;
    h->Init.AutoRetransmission = ENABLE;
    h->Init.AutoWakeUp = DISABLE;
    h->Init.ReceiveFifoLocked = DISABLE;
    h->Init.TransmitFifoPriority = DISABLE;

    switch (bitrate) {
    case 1000000: h->Init.Prescaler = 3; break;   /* 36MHz/(3*12) = 1Mbps */
    case 500000:  h->Init.Prescaler = 6; break;
    case 250000:  h->Init.Prescaler = 12; break;
    case 125000:  h->Init.Prescaler = 24; break;
    default:
        return -1;
    }

    if (HAL_CAN_Init(h) != HAL_OK)
        return -1;
    if (HAL_CAN_Start(h) != HAL_OK)
        return -1;

    return 0;
}

int bx_send(struct bxcan_driver *drv, const struct can_frame *frm)
{
    if (!drv || !drv->hcan || !frm)
        return -1;

    CAN_HandleTypeDef *h = drv->hcan;
    CAN_TxHeaderTypeDef header;
    uint32_t mailbox;

    header.DLC = frm->dlc;
    header.IDE = (frm->id > 0x7FF) ? CAN_ID_EXT : CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.TransmitGlobalTime = DISABLE;
    if (header.IDE == CAN_ID_EXT)
        header.ExtId = frm->id;
    else
        header.StdId = frm->id & 0x7FF;

    if (HAL_CAN_AddTxMessage(h, &header, (uint8_t *)frm->data, &mailbox) != HAL_OK)
        return -1;

    /* Wait for transmission completion */
    uint32_t tickstart = HAL_GetTick();
    while (HAL_CAN_IsTxMessagePending(h, mailbox)) {
        if ((HAL_GetTick() - tickstart) > 10) /* simple timeout */
            return -1;
    }
    return 0;
}

int bx_receive(struct bxcan_driver *drv, struct can_frame *frm)
{
    if (!drv || !drv->hcan || !frm)
        return -1;

    CAN_HandleTypeDef *h = drv->hcan;
    CAN_RxHeaderTypeDef header;
    if (HAL_CAN_GetRxFifoFillLevel(h, CAN_RX_FIFO0) == 0)
        return -1; /* no data */

    if (HAL_CAN_GetRxMessage(h, CAN_RX_FIFO0, &header, frm->data) != HAL_OK)
        return -1;

    frm->dlc = header.DLC;
    if (header.IDE == CAN_ID_EXT)
        frm->id = header.ExtId;
    else
        frm->id = header.StdId;

    return 0;
}

int bx_set_filter(struct bxcan_driver *drv, int idx, uint32_t id, uint32_t mask)
{
    if (!drv || !drv->hcan)
        return -1;

    CAN_HandleTypeDef *h = drv->hcan;
    CAN_FilterTypeDef f;

    f.FilterBank = idx;
    f.FilterMode = CAN_FILTERMODE_IDMASK;
    f.FilterScale = CAN_FILTERSCALE_32BIT;
    f.FilterIdHigh     = id >> 16;
    f.FilterIdLow      = id & 0xFFFF;
    f.FilterMaskIdHigh = mask >> 16;
    f.FilterMaskIdLow  = mask & 0xFFFF;
    f.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    f.FilterActivation = ENABLE;
    f.SlaveStartFilterBank = 14;

    return (HAL_CAN_ConfigFilter(h, &f) == HAL_OK) ? 0 : -1;
}

int bx_set_mode(struct bxcan_driver *drv, int loopback, int silent)
{
    if (!drv || !drv->hcan)
        return -1;

    CAN_HandleTypeDef *h = drv->hcan;

    if (HAL_CAN_Stop(h) != HAL_OK)
        return -1;

    if (loopback && silent)
        h->Init.Mode = CAN_MODE_SILENT_LOOPBACK;
    else if (silent)
        h->Init.Mode = CAN_MODE_SILENT;
    else if (loopback)
        h->Init.Mode = CAN_MODE_LOOPBACK;
    else
        h->Init.Mode = CAN_MODE_NORMAL;

    if (HAL_CAN_Init(h) != HAL_OK)
        return -1;
    if (HAL_CAN_Start(h) != HAL_OK)
        return -1;

    return 0;
}

uint32_t bx_get_error(struct bxcan_driver *drv)
{
    if (!drv || !drv->hcan)
        return 0;
    return HAL_CAN_GetError(drv->hcan);
}

int bx_autobaud(struct bxcan_driver *drv)
{
    (void)drv;
    /* Autobaud detection is hardware specific and is not implemented */
    return -1;
}
